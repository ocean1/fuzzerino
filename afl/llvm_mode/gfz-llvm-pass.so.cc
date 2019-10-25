/*
   american fuzzy lop - LLVM-mode instrumentation pass
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres. C bits copied-and-pasted
   from afl-as.c are Michal's fault.

   Copyright 2015, 2016 Google Inc. All rights reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

   This library is plugged into LLVM when invoking clang through afl-clang-fast.
   It tells the compiler to add code roughly equivalent to the bits discussed
   in ../afl-as.h.

 */
#define AFL_LLVM_PASS
#define MESSAGES_TO_STDOUT

#include "../config.h"
#include "../debug.h"

#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/APInt.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/XRay/YAMLXRayRecord.h"
#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>

using llvm::xray::YAMLXRayTrace;
using llvm::yaml::Input;

using namespace llvm;

namespace {

class AFLCoverage : public ModulePass {

public:
  static char ID;
  AFLCoverage() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;

  // StringRef getPassName() const override {
  //  return "American Fuzzy Lop Instrumentation";
  // }

private:
  FILE *log_fd;

  /* Instrumented location ID and basic block ID.
     
     These are important and will be embedded in the final instrumented binary
     by the linker, in order to send them to the fuzzer.

     - num_locs is useful to know how many numeric locations have been instrumented.
     - ptr_locs is useful to know how many pointer locations have been instrumented.
     - branch_locs is useful to know how many branches have been instrumented.
     - total_bbs is useful to know the total number of basic blocks and measure
       the percentage of generator coverage. */
  uint32_t num_locs = 0;
  uint32_t ptr_locs = 0;
  uint32_t branch_locs = 0;
  uint32_t total_bbs = 0;

  /* Total number of instructions and number of instruction selected for instrumentation.
     Only for informative purposes. */
  uint32_t tot = 0;
  uint32_t sel = 0;
  
  DataLayout *DL;

  GlobalVariable *GFZNumMapPtr, *GFZPtrMapPtr, *GFZBranchMapPtr, *GFZPtrBufPtr,
                 *GFZCovMapPtr, *GFZRandPtr, *GFZRandIdx;

  Value* emitInstrumentation(LLVMContext &C, IRBuilder<> &IRB,
                             Value *Original, Value *MutationFlags,
                             bool isGEP = false);
  void instrumentOperands(Instruction *I);
  void instrumentResult(Instruction *I);
  void instrumentBranch(Instruction *I);
};

} // namespace

char AFLCoverage::ID = 0;

///
/// Shamelessly stolen from lib/Transforms/Scalar/ConstantHoisting.cpp
///
/// \brief Updates the operand at Idx in instruction Inst with the result of
///        instruction Mat. If the instruction is a PHI node then special
///        handling for duplicate values form the same incoming basic block is
///        required.
/// \return The update will always succeed, but the return value indicated if
///         Mat was used for the update or not.
static bool updateOperand(Instruction *Inst, unsigned Idx, Instruction *Mat) {
  // Actually I disabled instrumentation
  // of PHI instructions so this is useless
  if (auto PHI = dyn_cast<PHINode>(Inst)) {
    // Check if any previous operand of the PHI node has the same incoming basic
    // block. This is a very odd case that happens when the incoming basic block
    // has a switch statement. In this case use the same value as the previous
    // operand(s), otherwise we will fail verification due to different values.
    // The values are actually the same, but the variable names are different
    // and the verifier doesn't like that.
    BasicBlock *IncomingBB = PHI->getIncomingBlock(Idx);
    for (unsigned i = 0; i < Idx; ++i) {
      if (PHI->getIncomingBlock(i) == IncomingBB) {
        Value *IncomingVal = PHI->getIncomingValue(i);
        Inst->setOperand(Idx, IncomingVal);
        return false;
      }
    }
  }

  Inst->setOperand(Idx, Mat);
  return true;
}

/* Returns NULL if the pointer is *not*
   a GEPOperator or the result of a GetElementPtrInst. */

Type* getPointeeTy(Value *Op) {

  if ( isa<GEPOperator>(Op) )
    return dyn_cast<PointerType>(
             dyn_cast<GEPOperator>(Op)->getPointerOperandType()
           )->getElementType();

  if ( isa<GetElementPtrInst>(Op) )
    return dyn_cast<PointerType>(
             dyn_cast<GetElementPtrInst>(Op)->getPointerOperandType()
           )->getElementType();

  return NULL;

}

/* This function decides which pointers get instrumented
   and which don't, by enforcing some heuristics. */

bool checkPointer(Value *Op) {

  Type *PointeeTy = getPointeeTy(Op);

  /* Not GEPOperator or result of GEP.
     TODO: this way "load"-ed pointers are not instrumented
           but is a load _always_ paired with a GEP??*/
  if ( PointeeTy == NULL )
    return false;

  if ( !PointeeTy->isSized() )
    return false;

  // TODO implement this!
  // Multiple dereferencing... but for example argv is i8**,
  // probably cases like those are tricky
  if ( PointeeTy->isPointerTy() )
    return false;

  // this could be dynamic allocation...
  if ( PointeeTy->isIntegerTy() ) {
    // unsigned bits = PointeeTy->getIntegerBitWidth();
    return false;
  }

  return true;

}

Value* AFLCoverage::emitInstrumentation(LLVMContext &C, IRBuilder<> &IRB,
                                        Value *Original, Value *MutationFlags,
                                        bool isGEP) {
  
  Value *FuzzedVal = NULL;

  Type *Ty = Original->getType();

  // Very verbose part due to all the nested mul(s), lshr(s),
  // and(s), also the integer constants that should have the
  // correct bit width.
  //
  // Whenever we need an integer constant we call
  //
  //   ConstantInt::get(IntegerType::getIntNTy(C, bits), value))
  //

  if ( Ty->isIntegerTy() ) {

    unsigned bits = Ty->getIntegerBitWidth();

    // If the bits of the operand are not default (16),
    // zext or bitcast the MutationFlags to match them.
    if (bits != 16) {
      MutationFlags = IRB.CreateIntCast(MutationFlags,
        IntegerType::getIntNTy(C, bits), false);
    }
      
    // Remember to always get ConstantInt(s)
    // with the correct bit width!

    // Op * (MutationFlags & 0000 0000 0000 0001)
    Value *KeepOriginal = IRB.CreateMul(
      Original,
      IRB.CreateAnd(MutationFlags,
                    ConstantInt::get(IntegerType::getIntNTy(C, bits), GFZ_KEEP_ORIGINAL)));
    
    // 1 * ((MutationFlags & 0000 0000 0000 0010) >> 1)
    Value *PlusOne = IRB.CreateMul(
      ConstantInt::get(IntegerType::getIntNTy(C, bits), 1),
      IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                   ConstantInt::get(IntegerType::getIntNTy(C, bits), GFZ_PLUS_ONE)),
                     ConstantInt::get(IntegerType::getIntNTy(C, bits), 1)));

    // -1 * ((MutationFlags & 0000 0000 0000 0100) >> 2)
    Value *MinusOne = IRB.CreateMul(
      ConstantInt::get(IntegerType::getIntNTy(C, bits), -1),
      IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                   ConstantInt::get(IntegerType::getIntNTy(C, bits), GFZ_MINUS_ONE)),
                     ConstantInt::get(IntegerType::getIntNTy(C, bits), 2)));
    
    // Add mutations together to obtain the new operand
    FuzzedVal = IRB.CreateAdd(KeepOriginal,
                  IRB.CreateAdd(PlusOne, MinusOne));

    // Interesting values based on bit width
    std::vector<int> interesting;

    if (bits == 8) {
      int arr[] = { 4, 8, 16, 32, 64, 100, 128, 200 };
      interesting.insert(interesting.end(), arr, arr+(sizeof(arr)/sizeof(arr[0])));
    } else if (bits == 16) {
      int arr[] = { -129, 128, 256, 512, 1000, 1024, 2048, 4096 };
      interesting.insert(interesting.end(), arr, arr+(sizeof(arr)/sizeof(arr[0])));
    } else if (bits == 32) {
      int arr[] = { -32769, 2048, 4096, 8192, 10000, 16384, 32768, 65536 };
      interesting.insert(interesting.end(), arr, arr+(sizeof(arr)/sizeof(arr[0])));
    }

    unsigned flag = 8;
    unsigned shift_pos = 3;

    for ( int i : interesting ) {
      FuzzedVal = IRB.CreateAdd(FuzzedVal,
        IRB.CreateMul(
          ConstantInt::get(IntegerType::getIntNTy(C, bits), i),
          IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                       ConstantInt::get(IntegerType::getIntNTy(C, bits), flag)),
                         ConstantInt::get(IntegerType::getIntNTy(C, bits), shift_pos))));

      flag <<= 1;
      shift_pos++;
    }

    // If the instruction is a GEP, _avoid_
    // adding havoc values (max int, random)

    if ( !isGEP ) {
      
      // Create arbitrary precision int with all ones
      // except for sign bit, which is cleared
      APInt max_int = APInt(bits, 0, false);
      max_int.setAllBits();
      max_int.clearSignBit();

      // max * ((MutationFlags & 0000 1000 0000 0000) >> 11)
      Value *PlusMax = IRB.CreateMul(
        ConstantInt::get(C, max_int),
        IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                     ConstantInt::get(IntegerType::getIntNTy(C, bits), GFZ_PLUS_MAX)),
                       ConstantInt::get(IntegerType::getIntNTy(C, bits), 11)));

      /* Access random pool */
      PointerType *PT = PointerType::getUnqual(Ty);
      LoadInst *RandIdx = IRB.CreateLoad(GFZRandIdx);
      LoadInst *RandPtr = IRB.CreateLoad(GFZRandPtr);
      Value *RandPtrCast = IRB.CreateZExtOrBitCast(RandPtr, PT);
      Value *RandPtrIdx = IRB.CreateGEP(RandPtrCast, RandIdx);
      LoadInst *RandVal = IRB.CreateLoad(RandPtrIdx);

      /* Increase idx to access random pool */
      Value *Incr = IRB.CreateAdd(RandIdx, ConstantInt::get(IntegerType::getInt32Ty(C), 1));
      Incr = IRB.CreateURem(Incr, ConstantInt::get(IntegerType::getInt32Ty(C), 4096));
      IRB.CreateStore(Incr, GFZRandIdx);

      // rand() * ((MutationFlags & 0001 0000 0000 0000) >> 12)
      Value *PlusRand = IRB.CreateMul(
        RandVal,
        IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                     ConstantInt::get(IntegerType::getIntNTy(C, bits), GFZ_PLUS_RAND)),
                       ConstantInt::get(IntegerType::getIntNTy(C, bits), 12)));
      
      // Add all these values together to obtain the new operand
      FuzzedVal = IRB.CreateAdd(FuzzedVal,
                    IRB.CreateAdd(PlusMax, PlusRand));
    }
  
  } else if ( Ty->isPointerTy() ) {

    // ((MutationFlags & 0000 0000 0010 0000) >> 5)
    Value *CustomBuf = IRB.CreateLShr(
      IRB.CreateAnd(MutationFlags,
                    ConstantInt::get(IntegerType::getInt16Ty(C), GFZ_CUSTOM_BUF)),
      ConstantInt::get(IntegerType::getInt16Ty(C), 5));

    Value *CustomBufEnabled = IRB.CreateICmpNE(CustomBuf,
                                               ConstantInt::get(IntegerType::getInt16Ty(C), 0));

    TerminatorInst *ThenTerm = NULL;
    TerminatorInst *ElseTerm = NULL;

    SplitBlockAndInsertIfThenElse(CustomBufEnabled, &(*(IRB.GetInsertPoint())),
      &ThenTerm, &ElseTerm, MDBuilder(C).createBranchWeights(1, (1<<20)-1));

    // "then" block - executed if GFZ_CUSTOM_BUF is active
    IRB.SetInsertPoint(ThenTerm);

    Type *PointeeTy = getPointeeTy(Original);

    IRB.CreateMemCpy(Original,
                     IRB.CreateLoad(GFZPtrBufPtr),
                     DL->getTypeStoreSize(PointeeTy)-1, 0);

    // "else" block - every other ptr mutation
    IRB.SetInsertPoint(ElseTerm);

    /* Cast stuff */

    MutationFlags = IRB.CreateIntCast(MutationFlags,
      IntegerType::getInt32Ty(C), false);

    Original = IRB.CreateZExtOrBitCast(Original,
      PointerType::getUnqual(IntegerType::getInt32Ty(C)));

    /* Load length and stride */

    // Len = ((MutationFlags & 0000 0001 1100 0000) >> 6)
    Value *Len = IRB.CreateLShr(
      IRB.CreateAnd(MutationFlags,
                    ConstantInt::get(IntegerType::getInt32Ty(C), 448)),
      ConstantInt::get(IntegerType::getInt32Ty(C), 6));

    // Stride = ((MutationFlags & 1111 1110 0000 0000) >> 9)
    Value *Stride = IRB.CreateLShr(
      IRB.CreateAnd(MutationFlags,
                    ConstantInt::get(IntegerType::getInt32Ty(C), 65024)),
      ConstantInt::get(IntegerType::getInt32Ty(C), 9));

    // (0xFF >> Len) * ((MutationFlags & 0000 0000 0000 0010) >> 1)
    Value *BitFlip = IRB.CreateMul(
      IRB.CreateLShr(ConstantInt::get(IntegerType::getInt32Ty(C), 0xFF),
                     Len),
      IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                   ConstantInt::get(IntegerType::getInt32Ty(C), GFZ_BITFLIP)),
                     ConstantInt::get(IntegerType::getInt32Ty(C), 1)));

    // (0xFFFFFFFF >> (Len * 8)) * ((MutationFlags & 0000 0000 0000 0100) >> 2)
    Value *ByteFlip = IRB.CreateMul(
      IRB.CreateLShr(ConstantInt::get(IntegerType::getInt32Ty(C), 0xFFFFFFFF),
                     IRB.CreateMul(Len,
                                   ConstantInt::get(IntegerType::getInt32Ty(C), 8))),
      IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                   ConstantInt::get(IntegerType::getInt32Ty(C), GFZ_BYTEFLIP)),
                     ConstantInt::get(IntegerType::getInt32Ty(C), 2)));

    // (Len + 1) * ((MutationFlags & 0000 0000 0000 1000) >> 3)
    Value *Arith = IRB.CreateMul(
      IRB.CreateAdd(Len,
                    ConstantInt::get(IntegerType::getInt32Ty(C), 1)),
      IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                   ConstantInt::get(IntegerType::getInt32Ty(C), GFZ_ARITH)),
                     ConstantInt::get(IntegerType::getInt32Ty(C), 3)));

    // ?? * ((MutationFlags & 0000 0000 00010 000) >> 4)
    /*Value *Interesting = IRB.CreateMul(
      ??,
      IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                   ConstantInt::get(IntegerType::getInt32Ty(C), GFZ_INTERESTING)),
                     ConstantInt::get(IntegerType::getInt32Ty(C), 4)));*/

    /* Apply stride - 1 byte version*/

    Original = IRB.CreatePointerCast(
      IRB.CreateGEP(IRB.CreatePointerCast(Original,
                                          PointerType::getUnqual(IntegerType::getInt8Ty(C))),
                    Stride),
      Original->getType());

    /* Apply stride - 4 byte version
       not suitable for bitflips - they'd skip 3 bytes */

    // Original = IRB.CreateGEP(Original, Stride);

    /* Xor original buffer content and mutations together
       to obtain the new buffer content */
    
    Value *ToStore = IRB.CreateAdd(
                       IRB.CreateXor(
                         IRB.CreateLoad(Original),
                           IRB.CreateXor(BitFlip, ByteFlip)),
                       Arith);

    /* Store new buffer content */
    
    IRB.CreateStore(ToStore, Original);

  } else if ( Ty->isFloatingPointTy() ) {
  
    // Op * (MutationFlags & 0000 0000 0000 0001)
    Value *KeepOriginal = IRB.CreateFMul(
      Original,
      IRB.CreateUIToFP(
        IRB.CreateAnd(MutationFlags,
                      ConstantInt::get(IntegerType::getInt16Ty(C), GFZ_KEEP_ORIGINAL)),
        Original->getType()));
    
    // 1 * ((MutationFlags & 0000 0000 0000 0010) >> 1)
    Value *PlusOne = IRB.CreateFMul(
      ConstantFP::get(Original->getType(), 1.0),
      IRB.CreateUIToFP(
        IRB.CreateLShr(
          IRB.CreateAnd(MutationFlags,
                        ConstantInt::get(IntegerType::getInt16Ty(C), GFZ_PLUS_ONE)),
          ConstantInt::get(IntegerType::getInt16Ty(C), 1)),
        Original->getType()));

    // -1 * ((MutationFlags & 0000 0000 0000 0100) >> 2)
    Value *MinusOne = IRB.CreateFMul(
      ConstantFP::get(Original->getType(), -1.0),
      IRB.CreateUIToFP(
        IRB.CreateLShr(
          IRB.CreateAnd(MutationFlags,
                        ConstantInt::get(IntegerType::getInt16Ty(C), GFZ_MINUS_ONE)),
          ConstantInt::get(IntegerType::getInt16Ty(C), 2)),
        Original->getType()));
    
    // Add mutations together to obtain the new operand
    FuzzedVal = IRB.CreateFAdd(KeepOriginal,
                  IRB.CreateFAdd(PlusOne, MinusOne));

    // Interesting values
    std::vector<double> interesting;

    double arr[] = { -32769.0, -129.0, 4.0, 8.0, 16.0, 32.0, 64.0, 100.0 };
    interesting.insert(interesting.end(), arr, arr+(sizeof(arr)/sizeof(arr[0])));
    
    unsigned flag = 8;
    unsigned shift_pos = 3;

    for ( double i : interesting ) {
      FuzzedVal = IRB.CreateFAdd(FuzzedVal,
        IRB.CreateFMul(
          ConstantFP::get(Original->getType(), i),
          IRB.CreateUIToFP(
            IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                         ConstantInt::get(IntegerType::getInt16Ty(C), flag)),
                           ConstantInt::get(IntegerType::getInt16Ty(C), shift_pos)),
            Original->getType())));

      flag <<= 1;
      shift_pos++;
    }

    // infinity * ((MutationFlags & 0000 1000 0000 0000) >> 11)
    Value *PlusMax = IRB.CreateFMul(
      ConstantFP::get(Original->getType(), FLT_MAX),
      IRB.CreateUIToFP(
        IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                     ConstantInt::get(IntegerType::getInt16Ty(C), GFZ_PLUS_MAX)),
                       ConstantInt::get(IntegerType::getInt16Ty(C), 11)),
        Original->getType()));

    // Access random pool
    PointerType *PT = PointerType::getUnqual(IntegerType::getInt32Ty(C));
    LoadInst *RandIdx = IRB.CreateLoad(GFZRandIdx);
    LoadInst *RandPtr = IRB.CreateLoad(GFZRandPtr);
    Value *RandPtrCast = IRB.CreateZExtOrBitCast(RandPtr, PT);
    Value *RandPtrIdx = IRB.CreateGEP(RandPtrCast, RandIdx);
    LoadInst *RandVal = IRB.CreateLoad(RandPtrIdx);

    // Increase idx to access random pool
    Value *Incr = IRB.CreateAdd(RandIdx, ConstantInt::get(IntegerType::getInt32Ty(C), 1));
    Incr = IRB.CreateURem(Incr, ConstantInt::get(IntegerType::getInt32Ty(C), 4096));
    IRB.CreateStore(Incr, GFZRandIdx);

    // rand() * ((MutationFlags & 0001 0000 0000 0000) >> 12)
    Value *PlusRand = IRB.CreateFMul(
      IRB.CreateUIToFP(RandVal, Original->getType()),
      IRB.CreateUIToFP(  
        IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                     ConstantInt::get(IntegerType::getInt16Ty(C), GFZ_PLUS_RAND)),
                       ConstantInt::get(IntegerType::getInt16Ty(C), 12)),
        Original->getType()));
    
    // Add all these values together to obtain the new operand
    FuzzedVal = IRB.CreateFAdd(FuzzedVal,
                  IRB.CreateFAdd(PlusMax, PlusRand));

  }

  return FuzzedVal;

}

bool isOrContainsStructTy(Type *Ty) {

  if ( Ty->isStructTy() ) {
    return true;
  }

  bool result = false;
  unsigned numContainedTys = Ty->getNumContainedTypes();

  if ( numContainedTys > 0 ) {
    unsigned i = 0;

    for (i = 0; i < numContainedTys; ++i)
      result = result || isOrContainsStructTy(Ty->getContainedType(i));
  }

  return result;

}

void AFLCoverage::instrumentOperands(Instruction *I) {
  
  Function *F = I->getFunction();
  Module *M = F->getParent();
  LLVMContext &C = M->getContext();

  IntegerType *Int16Ty = IntegerType::getInt16Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);
  
  User::op_iterator OI, OE;

  bool isGEP = isa<GetElementPtrInst>(I);

  // Don't instrument GEP indices of struct types!
  if ( isGEP ) {

    GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(I);
    if ( GEPI == NULL ) return;

    Type *POT = GEPI->getPointerOperandType();
    if ( POT == NULL ) return;

    PointerType *PPOT = dyn_cast<PointerType>(POT);
    if ( PPOT == NULL ) return;

    Type *X = PPOT->getElementType();
    if ( X == NULL || isOrContainsStructTy(X) ) return;

  }

  auto *DI = dyn_cast<CallInst>(I);
  int op_idx;
  for ( op_idx = 0,
        OI = DI ? DI->arg_begin() : I->op_begin(),
        OE = DI ? DI->arg_end()   : I->op_end();
        OI != OE;
        ++OI, ++op_idx ) {

    if (OI == OE)
      break;

    auto *Op = dyn_cast<Value>(*OI);
    Op = Op ? Op : dyn_cast<Instruction>(*OI);
    
    if (!Op)
      continue;

    Type *OT = Op->getType();
    bool is_pointer = OT->getTypeID() == Type::TypeID::PointerTyID;

    std::string str;
    raw_string_ostream rso(str);
    OT->print(rso);
    if ( StringRef(rso.str()).contains(StringRef("IO_FILE")) )
      continue;

    // Don't instrument _pointer_ operands of GEP instructions,
    // at least until the point where the result is used
    if ( isGEP && OT->isPointerTy() )
      continue;

    // If the pointer is not 'eligible' for instrumentation,
    // skip emitting the IR for loading from gfz map, etc...
    if ( OT->isPointerTy() && !checkPointer(Op) )
      continue;

    IRBuilder<> IRB(I);

    // Use ID to reference the location the in map
    ConstantInt *InstID = ConstantInt::get(Int32Ty, is_pointer ? ptr_locs : num_locs);

    LoadInst *MapPtr = IRB.CreateLoad(is_pointer ? GFZPtrMapPtr : GFZNumMapPtr);
    Value *MapPtrIdx = IRB.CreateGEP(MapPtr, InstID);
    
    // MutationFlags tell us which mutations should be applied
    // to this location. Each bit is tied to a different mutation.
    //
    // See documentation for details.
    Value *MutationFlags = IRB.CreateLoad(MapPtrIdx);

    Value *FuzzedVal = NULL;

    // if ( MutationFlags != 1 )
    Value *InstEnabled = IRB.CreateICmpNE(MutationFlags, ConstantInt::get(Int16Ty, 1));

    TerminatorInst *CheckStatus = SplitBlockAndInsertIfThen(InstEnabled, I, false,
            MDBuilder(C).createBranchWeights(1, (1<<20)-1));

    // "then" block - executed if location is active
    IRB.SetInsertPoint(CheckStatus);
    FuzzedVal = emitInstrumentation(C, IRB, Op, MutationFlags, isGEP);

    if (FuzzedVal != NULL) { // FuzzedVal is NULL only for ptr instrumentation
      // Common block - executed anyway
      IRB.SetInsertPoint(I);
      PHINode *PHI = IRB.CreatePHI(OT, 2);
      
      BasicBlock *LoadBlock = MapPtr->getParent();
      PHI->addIncoming(Op, LoadBlock);
      
      BasicBlock *ThenBlock = dyn_cast<Instruction>(FuzzedVal)->getParent();
      PHI->addIncoming(FuzzedVal, ThenBlock);

      FuzzedVal = PHI;
      
      updateOperand(I, op_idx, dyn_cast<Instruction>(FuzzedVal));
    }

    is_pointer ? ptr_locs++ : num_locs++;

    /* BEGIN debugging stuff */

    std::string strr;
    raw_string_ostream rsoo(strr);
    I->print(rsoo);

    const llvm::DebugLoc &debugInfo = I->getDebugLoc();
    DILocation *DI = debugInfo.get();
    
    fprintf(log_fd, "\n(%s location %d):", is_pointer ? "ptr" : "num", (is_pointer ? ptr_locs : num_locs) - 1);

    if (DI != NULL) {
      std::string filePath = debugInfo->getFilename();
      int line = debugInfo->getLine();
      int column = debugInfo->getColumn();
      fprintf(log_fd, "\t%s, %d:%d, operand %d (%s)", filePath.c_str(), line, column, op_idx + 1, rsoo.str().c_str());
    } else {
      fprintf(log_fd, "\t??, operand %d (%s)", op_idx + 1, rsoo.str().c_str());
    }

    /* END debugging stuff */

  } // end op loop
} // end instrumentOperands

void AFLCoverage::instrumentResult(Instruction *I) {

  Function *F = I->getFunction();
  Module *M = F->getParent();
  LLVMContext &C = M->getContext();

  IntegerType *Int16Ty = IntegerType::getInt16Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  Type *IT = I->getType();
  bool is_pointer = IT->getTypeID() == Type::TypeID::PointerTyID;

  Instruction *NI = I->getNextNode();

  if (!NI)
    return;

  // Don't instrument results of instructions that are not used anywhere...
  if ( I->getNumUses() == 0 )
    return;

  std::string str;
  raw_string_ostream rso(str);
  IT->print(rso);
  if ( StringRef(rso.str()).contains(StringRef("IO_FILE")) )
    return;

  // If the pointer is not 'eligible' for instrumentation,
  // skip emitting the IR for loading from gfz map, etc...
  if ( IT->isPointerTy() && !checkPointer(I) )
    return;

  IRBuilder<> IRB(NI);

  UnreachableInst *FakeVal = IRB.CreateUnreachable();
  I->replaceAllUsesWith(FakeVal);

  // Use ID to reference the location in the map
  ConstantInt *InstID = ConstantInt::get(Int32Ty, is_pointer ? ptr_locs : num_locs);

  LoadInst *MapPtr = IRB.CreateLoad(is_pointer ? GFZPtrMapPtr : GFZNumMapPtr);
  Value *MapPtrIdx = IRB.CreateGEP(MapPtr, InstID);
  
  // MutationFlags tell us which mutations should be applied
  // to this location. Each bit is tied to a different mutation.
  //
  // See documentation for details.
  Value *MutationFlags = IRB.CreateLoad(MapPtrIdx);

  Value *FuzzedVal = NULL;

  // if ( MutationFlags != 1 )
  Value *InstEnabled = IRB.CreateICmpNE(MutationFlags, ConstantInt::get(Int16Ty, 1));

  TerminatorInst *CheckStatus = SplitBlockAndInsertIfThen(InstEnabled, NI, false,
          MDBuilder(C).createBranchWeights(1, (1<<20)-1));

  // "then" block - executed if location is active
  IRB.SetInsertPoint(CheckStatus);
  FuzzedVal = emitInstrumentation(C, IRB, I, MutationFlags);

  // Common block - executed anyway
  IRB.SetInsertPoint(NI);
  PHINode *PHI = IRB.CreatePHI(IT, 2);
  
  BasicBlock *LoadBlock = MapPtr->getParent();
  PHI->addIncoming(I, LoadBlock);

  BasicBlock *ThenBlock = dyn_cast<Instruction>(FuzzedVal)->getParent();
  PHI->addIncoming(FuzzedVal, ThenBlock);

  FakeVal->replaceAllUsesWith(PHI);
  FakeVal->eraseFromParent();

  is_pointer ? ptr_locs++ : num_locs++;

  /* BEGIN debugging stuff */

  std::string strr;
  raw_string_ostream rsoo(strr);
  I->print(rsoo);

  const llvm::DebugLoc &debugInfo = I->getDebugLoc();
  DILocation *DI = debugInfo.get();
  
  fprintf(log_fd, "\n(%s location %d):", is_pointer ? "ptr" : "num", (is_pointer ? ptr_locs : num_locs) - 1);

  if (DI != NULL) {
    std::string filePath = debugInfo->getFilename();
    int line = debugInfo->getLine();
    int column = debugInfo->getColumn();
    fprintf(log_fd, "\t%s, %d:%d, result (%s)", filePath.c_str(), line, column, rsoo.str().c_str());
  } else {
    fprintf(log_fd, "\t??, result (%s)", rsoo.str().c_str());
  }

  /* END debugging stuff */

} // end instrumentResult

void AFLCoverage::instrumentBranch(Instruction *I) {

  Function *F = I->getFunction();
  Module *M = F->getParent();
  LLVMContext &C = M->getContext();

  IntegerType *Int8Ty = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  if ( !isa<BranchInst>(I) )
    return;

  BranchInst *BI = dyn_cast<BranchInst>(I);

  if (!BI->isConditional())
    return;

  Value *Cond = BI->getCondition();
  Type *CT = Cond->getType();

  IRBuilder<> IRB(BI);

  // Use branch_locs to reference the location in the map
  ConstantInt *InstID = ConstantInt::get(Int32Ty, branch_locs);

  LoadInst *MapPtr = IRB.CreateLoad(GFZBranchMapPtr);
  Value *MapPtrIdx = IRB.CreateGEP(MapPtr, InstID);
  
  // MutationFlags tell us which mutations should be applied
  // to this location. Each bit is tied to a different mutation.
  //
  // See documentation for details.
  Value *MutationFlags = IRB.CreateLoad(MapPtrIdx);

  // if ( MutationFlags != 1 )
  Value *InstEnabled = IRB.CreateICmpNE(MutationFlags, ConstantInt::get(Int8Ty, 1));

  TerminatorInst *CheckStatus = SplitBlockAndInsertIfThen(InstEnabled, BI, false,
          MDBuilder(C).createBranchWeights(1, (1<<20)-1));

  // "then" block - executed if location is active
  IRB.SetInsertPoint(CheckStatus);
  
  Value *NotCond = IRB.CreateNot(Cond);

  // Common block - executed anyway
  IRB.SetInsertPoint(BI);
  PHINode *PHI = IRB.CreatePHI(CT, 2);
  
  BasicBlock *LoadBlock = MapPtr->getParent();
  PHI->addIncoming(Cond, LoadBlock);
  
  BasicBlock *ThenBlock = dyn_cast<Instruction>(NotCond)->getParent();
  PHI->addIncoming(NotCond, ThenBlock);

  BI->setCondition(PHI);

  branch_locs++;

  /* BEGIN debugging stuff */

  const llvm::DebugLoc &debugInfo = BI->getDebugLoc();
  DILocation *DI = debugInfo.get();
  
  fprintf(log_fd, "\n(bnc location %d):", branch_locs - 1);

  if (DI != NULL) {
    std::string filePath = debugInfo->getFilename();
    int line = debugInfo->getLine();
    int column = debugInfo->getColumn();
    fprintf(log_fd, "\t%s, %d:%d", filePath.c_str(), line, column);
  } else {
    fprintf(log_fd, "\t??");
  }

  /* END debugging stuff */

} // end instrumentBranch



bool AFLCoverage::runOnModule(Module &M) {

  log_fd = fopen("locations.log", "a");

  LLVMContext &C = M.getContext();

  IntegerType *Int8Ty = IntegerType::getInt8Ty(C);
  IntegerType *Int16Ty = IntegerType::getInt16Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  DL = new DataLayout(&M);
  
  GFZNumMapPtr = new GlobalVariable(M, PointerType::get(Int16Ty, 0), false,
                                    GlobalValue::ExternalLinkage, 0, "__gfz_num_map");

  GFZPtrMapPtr = new GlobalVariable(M, PointerType::get(Int16Ty, 0), false,
                                    GlobalValue::ExternalLinkage, 0, "__gfz_ptr_map");

  GFZBranchMapPtr = new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                                       GlobalValue::ExternalLinkage, 0, "__gfz_branch_map");

  GFZPtrBufPtr = new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                                    GlobalValue::ExternalLinkage, 0, "__gfz_ptr_buf");

  GFZCovMapPtr = new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                                    GlobalValue::ExternalLinkage, 0, "__gfz_cov_map");

  GFZRandPtr = new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                                  GlobalValue::ExternalLinkage, 0, "__gfz_rand_area");

  GFZRandIdx = new GlobalVariable(M, Int32Ty, false,
                                  GlobalValue::ExternalLinkage, 0, "__gfz_rand_idx");

  StringSet<> WhitelistSet;
  SmallVector<Instruction*, 64> toInstrumentBranches, toInstrumentOperands, toInstrumentResult;

  // Show a banner

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {
    SAYF(cCYA "gfz-llvm-pass " cBRI VERSION cRST
              " by <lszekeres@google.com>, <_ocean>, <limi7break>\n");
  } else {
    be_quiet = 1;
  }

  // Get temporary file to keep ID of fuzzed register to be used as gfz_map idx
  
  struct flock lock;
  int idfd;

  idfd = open(GFZ_IDFILE, O_CREAT | O_RDWR | O_NOATIME,
              S_IRUSR | S_IWUSR | S_IRGRP);
  memset(&lock, 0, sizeof(lock));

  // Whitelist functions: scan for functions explicitly marked as fuzzable
  
  for (auto &F : M)
    if (F.getSection() == ".fuzzables")
      WhitelistSet.insert(F.getName());

  WhitelistSet.insert(StringRef("png_set_packing"));
  WhitelistSet.insert(StringRef("png_set_bKGD"));
  WhitelistSet.insert(StringRef("png_set_compression_level"));
  WhitelistSet.insert(StringRef("png_init_io"));
  WhitelistSet.insert(StringRef("png_set_IHDR"));
  WhitelistSet.insert(StringRef("png_set_tIME"));
  WhitelistSet.insert(StringRef("png_write_info"));
  WhitelistSet.insert(StringRef("png_set_gAMA"));
  WhitelistSet.insert(StringRef("png_set_text"));
  WhitelistSet.insert(StringRef("png_set_text_2"));
  WhitelistSet.insert(StringRef("png_set_gAMA_fixed"));
  WhitelistSet.insert(StringRef("png_write_sCAL_s"));
  WhitelistSet.insert(StringRef("png_write_info_before_PLTE"));
  WhitelistSet.insert(StringRef("png_write_tRNS"));
  WhitelistSet.insert(StringRef("png_write_eXIf"));
  WhitelistSet.insert(StringRef("png_write_sPLT"));
  WhitelistSet.insert(StringRef("png_write_iTXt"));
  WhitelistSet.insert(StringRef("png_write_zTXt"));
  WhitelistSet.insert(StringRef("png_write_tEXt"));
  WhitelistSet.insert(StringRef("png_write_PLTE"));
  WhitelistSet.insert(StringRef("png_write_bKGD"));
  WhitelistSet.insert(StringRef("png_write_hIST"));
  WhitelistSet.insert(StringRef("png_write_IHDR"));
  WhitelistSet.insert(StringRef("png_write_pCAL"));
  WhitelistSet.insert(StringRef("png_write_oFFs"));
  WhitelistSet.insert(StringRef("png_write_pHYs"));
  WhitelistSet.insert(StringRef("png_write_sig"));
  WhitelistSet.insert(StringRef("write_unknown_chunks"));
  WhitelistSet.insert(StringRef("png_chunk_report"));
  WhitelistSet.insert(StringRef("png_colorspace_sync_info"));
  WhitelistSet.insert(StringRef("png_get_libpng_ver"));
  WhitelistSet.insert(StringRef("png_get_header_ver"));
  WhitelistSet.insert(StringRef("png_fixed"));
  WhitelistSet.insert(StringRef("writepng_init"));

  char *whitelist = getenv("GFZ_WHITE_LIST");

  // Read executed functions from llvm xray yaml
  
  if (whitelist) {
    printf("[*] Using whitelist.");
    std::ifstream t(whitelist);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    Input y(str);
    YAMLXRayTrace Wlt;
    y >> Wlt;

    for (auto const &rec : Wlt.Records) {
      WhitelistSet.insert(rec.Function);
    }
  }

  // Instrument all the things!

  int inst_fun = 0;  /* Total number of instrumented functions. Only for informative purposes. */
  
  int mod_tot = 0;   /* Total number of instructions in the module */
  int mod_sel = 0;   /* Number of instructions in the module that are selected for instrumentation */
  int fun_tot = 0;   /* Total number of instructions in the function */
  int fun_sel = 0;   /* Number of instructions in the function that are selected for instrumentation */

  // ugly hack, don't want to implement an LTO pass,
  // keep track of current ID in a locked file
  lock.l_type = F_WRLCK;
  fcntl(idfd, F_SETLKW, &lock);
  lseek(idfd, 0, SEEK_SET);

  if (read(idfd, &num_locs, sizeof(num_locs)) != sizeof(num_locs))
    num_locs = 0;

  if (read(idfd, &ptr_locs, sizeof(ptr_locs)) != sizeof(ptr_locs))
    ptr_locs = 0;

  if (read(idfd, &branch_locs, sizeof(branch_locs)) != sizeof(branch_locs))
    branch_locs = 0;

  if (read(idfd, &total_bbs, sizeof(total_bbs)) != sizeof(total_bbs))
    total_bbs = 0;

  if (read(idfd, &tot, sizeof(tot)) != sizeof(tot))
    tot = 0;

  if (read(idfd, &sel, sizeof(sel)) != sizeof(sel))
    sel = 0;

  SAYF("\n=== Module %s\n", M.getName().str().c_str());

  bool used_in_branch = false;

  for (auto &F : M) { // function loop
    
    fun_tot = 0;
    fun_sel = 0;

    if (F.getName().equals(StringRef("main"))) {
      GlobalVariable* GFZArgc
        = new GlobalVariable(M, Int32Ty, false,
                             GlobalValue::ExternalLinkage, 0, "__gfz_argc");

      GlobalVariable* GFZArgv
        = new GlobalVariable(M, PointerType::get(PointerType::get(Int8Ty, 0), 0), false,
                             GlobalValue::ExternalLinkage, 0, "__gfz_argv");

      BasicBlock::iterator IP = F.getEntryBlock().getFirstInsertionPt();
      IRBuilder<> IRB(&(*IP));
      
      for(auto A = F.arg_begin(); A != F.arg_end(); ++A)
        if (A->getArgNo() == 0)      A->replaceAllUsesWith(IRB.CreateLoad(GFZArgc));
        else if (A->getArgNo() == 1) A->replaceAllUsesWith(IRB.CreateLoad(GFZArgv));
    }

    if (WhitelistSet.find(F.getName()) == WhitelistSet.end())
      continue;

    if (F.isIntrinsic())
      continue;

    for (auto &BB : F) { // basic block loop

      if (BB.getFirstInsertionPt() == BB.end())
        continue;

      for (auto &I : BB) { // instruction loop

        fun_tot++;

        if ( isa<BranchInst>(I) ) {
          toInstrumentBranches.push_back(&I);
          fun_sel++;
          continue;
        }

        // is it used in a branch instruction?
        used_in_branch = false; 
        for ( auto U : I.users() ) {
          if ( isa<BranchInst>(U) ) {
            used_in_branch = true;
            break;
          }
        }
        if (used_in_branch)
          continue;

        // only instrument result (if not used in a branch instruction)
        if ( isa<SelectInst>(I) || isa<CmpInst>(I) ) {
          Type *IT = I.getType();
          
          if (! (IT->isIntegerTy() || IT->isFloatingPointTy() ) )
            continue;
          
          toInstrumentResult.push_back(&I);
          fun_sel++;
          continue;
        }

        // instrument blacklist
        if ( isa<IntrinsicInst>(I) || isa<SwitchInst>(I) ||
             isa<IndirectBrInst>(I) || isa<CatchSwitchInst>(I) ||
             isa<CatchReturnInst>(I) || isa<CleanupReturnInst>(I) ||
             isa<UnreachableInst>(I) || isa<ShuffleVectorInst>(I) ||
             isa<AllocaInst>(I) || isa<FenceInst>(I) ||
             isa<CastInst>(I) || isa<PHINode>(I) ||
             isa<VAArgInst>(I) || isa<LandingPadInst>(I) ||
             isa<CatchPadInst>(I) || isa<CleanupPadInst>(I) ||
             isa<BranchInst>(I) || isa<SelectInst>(I) ||
             isa<CmpInst>(I) ) {
          continue;
        }

        toInstrumentOperands.push_back(&I);
        fun_sel++;

        // instrument result blacklist
        if ( isa<ReturnInst>(I) || isa<ResumeInst>(I) ||
             isa<InsertElementInst>(I) || isa<AtomicCmpXchgInst>(I) ||
             isa<AtomicRMWInst>(I) || isa<StoreInst>(I) ||
             isa<GetElementPtrInst>(I) ) {
          continue;
        }

        Type *IT = I.getType();

        if (! (IT->isIntegerTy() || IT->isFloatingPointTy() ) )
          continue;

        toInstrumentResult.push_back(&I);

      } // end instruction loop

      /* Emit basic block coverage instrumentation

         __gfz_cov_map[total_bbs] = 1; */

      BasicBlock::iterator IP = BB.getFirstInsertionPt();
      IRBuilder<> IRB(&(*IP));

      LoadInst *CovMap = IRB.CreateLoad(GFZCovMapPtr);
      Value *CovMapIdx = IRB.CreateGEP(CovMap, ConstantInt::get(Int32Ty, total_bbs));
      IRB.CreateStore(ConstantInt::get(Int8Ty, 1), CovMapIdx);

      total_bbs++;

    } // end basic block loop

    if (fun_sel) {
      inst_fun++;
      
      if (!be_quiet)
        OKF("Function %s: selected %u/%u instructions.", F.getName().str().c_str(), fun_sel, fun_tot);

    } else {

      if (!be_quiet)
        OKF("Function %s: skipped.", F.getName().str().c_str());

    }
  
    mod_tot += fun_tot;
    mod_sel += fun_sel;

  } // end function loop

  SAYF("\n");

  tot += mod_tot;
  sel += mod_sel;

  if (mod_sel) {
    OKF("Module %s: selected %u/%u instructions in %u function(s).", M.getName().str().c_str(), mod_sel, mod_tot, inst_fun);
    OKF("Total: selected %u/%u instructions.\n"
        "           %u basic blocks.", sel, tot, total_bbs);
  } else {
    OKF("Module %s: skipped.", M.getName().str().c_str());
  }

  // Pass the selected instructions to the 
  // corresponding instrumenting functions

  for (auto I: toInstrumentBranches)
    instrumentBranch(I);

  for (auto I: toInstrumentResult)
    instrumentResult(I);

  for (auto I: toInstrumentOperands)
    instrumentOperands(I);
  
  if (mod_sel)
    OKF("Total: instrumented %u numeric locations, %u pointer locations and %u branches.",
        num_locs, ptr_locs, branch_locs);

  // Write info that needs to be maintained between modules to GFZ_IDFILE

  lseek(idfd, 0, SEEK_SET);
  
  if (write(idfd, &num_locs, sizeof(num_locs)) != sizeof(num_locs))
    FATAL("Got a problem while writing num_locs on %s", GFZ_IDFILE);

  if (write(idfd, &ptr_locs, sizeof(ptr_locs)) != sizeof(ptr_locs))
    FATAL("Got a problem while writing ptr_locs on %s", GFZ_IDFILE);

  if (write(idfd, &branch_locs, sizeof(branch_locs)) != sizeof(branch_locs))
    FATAL("Got a problem while writing branch_locs on %s", GFZ_IDFILE);

  if (write(idfd, &total_bbs, sizeof(total_bbs)) != sizeof(total_bbs))
    FATAL("Got a problem while writing total_bbs on %s", GFZ_IDFILE);

  if (write(idfd, &tot, sizeof(tot)) != sizeof(tot))
    FATAL("Got a problem while writing total instructions on %s", GFZ_IDFILE);

  if (write(idfd, &sel, sizeof(sel)) != sizeof(sel))
    FATAL("Got a problem while writing selected instructions on %s", GFZ_IDFILE);

  lock.l_type = F_UNLCK;
  fcntl(idfd, F_SETLKW, &lock);

  fclose(log_fd);

  return true;

}

static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLCoverage());
}

static RegisterStandardPasses
    RegisterAFLPass(PassManagerBuilder::EP_OptimizerLast, registerAFLPass);

static RegisterStandardPasses
    RegisterAFLPass0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                     registerAFLPass);