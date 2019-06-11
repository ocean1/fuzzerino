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
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/XRay/YAMLXRayRecord.h"
#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>

// #define branching_en

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
  uint32_t inst_id = 0;
  uint32_t tot = 0;
  uint32_t sel = 0;
  FILE *map_key_fd;
  void instrumentOperands(Instruction *I, GlobalVariable *GFZMapPtr,
                          GlobalVariable *GFZRandPtr, GlobalVariable *GFZRandIdx);
  Instruction* instrumentResult(Instruction *I, GlobalVariable *GFZMapPtr,
                                GlobalVariable *GFZRandPtr, GlobalVariable *GFZRandIdx);
  // Value *fuzzInt(IRBuilder<> IRB, Instruction *I);
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

void AFLCoverage::instrumentOperands(Instruction *I, GlobalVariable *GFZMapPtr,
                                     GlobalVariable *GFZRandPtr,
                                     GlobalVariable *GFZRandIdx) {
  
  Function *F = I->getFunction();
  Module *M = F->getParent();
  LLVMContext &C = M->getContext();

  // for each op, if type is Int (will need to add also for floats, and
  // other types....) take OI as Value, push fuzzed val, substitute it
  // val = insertFuzzVal(I, OI);
  // updateOp(OI, val);
  // Value *inval = dyn_cast<Value*>(OI);

  User::op_iterator OI, OE;

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

    if (! (OT->isIntegerTy() /*|| OT->isPointerTy() || OT->isFloatingPointTy()*/) )
      continue;

    IRBuilder<> IRB(I);

    // Use inst_id to reference the fuzzable location in __gfz_map_area
    ConstantInt *InstID = ConstantInt::get(IntegerType::getInt32Ty(C),
                                           inst_id);

    LoadInst *MapPtr = IRB.CreateLoad(GFZMapPtr);
    Value *MapPtrIdx = IRB.CreateGEP(MapPtr, InstID);
    
    // MutationFlags tell us which mutations should be applied
    // to this location. Each bit is tied to a different mutation.
    //
    // See documentation for details.
    Value *MutationFlags = IRB.CreateLoad(MapPtrIdx);

    // Very verbose part due to all the nested mul(s), lshr(s),
    // and(s), also the integer constants that should have the
    // correct bit width.
    //
    // Whenever we need an integer constant we call
    //
    //     ConstantInt::get(IntegerType::getIntNTy(C, bits), value))
    //
    Value *FuzzedVal = NULL;

    if ( OT->isIntegerTy() ) {

      unsigned bits = OT->getIntegerBitWidth();

      // If the bits of the operand are not default (16),
      // zext or bitcast the MutationFlags to match them.
      if (bits != 16) {
        MutationFlags = IRB.CreateIntCast(MutationFlags,
          IntegerType::getIntNTy(C, bits), false);
      }
        
      // Remember to always get ConstantInt(s)
      // with the correct bit width!

      // Op * (MutationFlags & 00000001)
      Value *KeepOriginal = IRB.CreateMul(
        Op,
        IRB.CreateAnd(MutationFlags,
                      ConstantInt::get(IntegerType::getIntNTy(C, bits), 1)));
      
      // 1 * ((MutationFlags & 00000010) >> 1)
      Value *PlusOne = IRB.CreateMul(
        ConstantInt::get(IntegerType::getIntNTy(C, bits), 1),
        IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                     ConstantInt::get(IntegerType::getIntNTy(C, bits), 2)),
                       ConstantInt::get(IntegerType::getIntNTy(C, bits), 1)));

      // -1 * ((MutationFlags & 00000100) >> 2)
      Value *MinusOne = IRB.CreateMul(
        ConstantInt::get(IntegerType::getIntNTy(C, bits), -1),
        IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                     ConstantInt::get(IntegerType::getIntNTy(C, bits), 4)),
                       ConstantInt::get(IntegerType::getIntNTy(C, bits), 2)));
      
      // Add mutations together to obtain the new operand
      FuzzedVal = IRB.CreateAdd(KeepOriginal,
                    IRB.CreateAdd(PlusOne, MinusOne));

      // Interesting values based on bit width
      std::vector<int> interesting;

      if (bits == 8) {
        int arr[] = { 4, 8, 16, 32, 64, 100, 128, 200};
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

      // If the instruction is a GEP, _avoid_ adding
      // max int value and random values.

      if ( !isa<GetElementPtrInst>(I) ) {
        
        // Create arbitrary precision int with all ones
        // except for sign bit, which is cleared
        APInt max_int = APInt(bits, 0, false);
        max_int.setAllBits();
        max_int.clearSignBit();

        // max * ((MutationFlags & 0100 0000 0000) >> 3)
        Value *PlusMax = IRB.CreateMul(
          ConstantInt::get(C, max_int),
          IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                       ConstantInt::get(IntegerType::getIntNTy(C, bits), 2048)),
                         ConstantInt::get(IntegerType::getIntNTy(C, bits), 11)));

        // Access random pool
        // Index increase is *not* performed here because we want to
        // avoid branches, to increase it only if the "PLUS_RAND" mutation
        // is enabled. The parent process "knows" if it is enabled from
        // the outside, so the increase can be performed there if needed.
        PointerType *PT = PointerType::getUnqual(OT);
        LoadInst *RandIdx = IRB.CreateLoad(GFZRandIdx);
        LoadInst *RandPtr = IRB.CreateLoad(GFZRandPtr);
        Value *RandPtrCast = IRB.CreateZExtOrBitCast(RandPtr, PT);
        Value *RandPtrIdx = IRB.CreateGEP(RandPtrCast, RandIdx);
        LoadInst *RandVal = IRB.CreateLoad(RandPtrIdx);

        // rand() * ((MutationFlags & 00010000) >> 4)
        Value *PlusRand = IRB.CreateMul(
          RandVal,
          IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                                       ConstantInt::get(IntegerType::getIntNTy(C, bits), 4096)),
                         ConstantInt::get(IntegerType::getIntNTy(C, bits), 12)));
        
        // Add all these values together to obtain the new operand
        FuzzedVal = IRB.CreateAdd(FuzzedVal,
                      IRB.CreateAdd(PlusMax, PlusRand));
      }
    
    } else if ( OT->isPointerTy() ) {

      // TODO
      //
      // probably it only makes sense to act on the
      // pointed buffer, but can I get info on it?
    
    } else if ( OT->isFloatingPointTy() ) {
    
      // TODO

    }

    // Debugging stuff    

    fprintf(map_key_fd, "\n%d:\t%s", inst_id, I->getOpcodeName());

    if ( isa<CallInst>(I) )
      fprintf(map_key_fd, " (%s)", dyn_cast<CallInst>(I)->getCalledFunction()->getName());

    fprintf(map_key_fd, ", operand %d", op_idx);

    std::string str;
    raw_string_ostream rso(str);
    OT->print(rso);
    fprintf(map_key_fd, ", type %s", rso.str().c_str());

    // Update operand and increase the id of the
    // instrumented location in the map
    
    updateOperand(I, op_idx, dyn_cast<Instruction>(FuzzedVal));
    inst_id++;

    // TODO: Custom mutations for each type of instruction?
    if ( isa<ExtractElementInst>(I) ) {
      // 2nd operand only (integer, index)
    } else if ( isa<InsertElementInst>(I) ) {
      // 2nd operand (whatever, 1st class value)
      // 3rd operand (integer, index)
    } else if ( isa<ExtractValueInst>(I) ) {
      // index/indices (integer(s)) from 2nd onwards
    } else if ( isa<InsertValueInst>(I) ) {
      // value (whatever, 1st class value) (2nd)
      // and index/indices (integer(s)) from 3rd onwards
    } else if ( isa<AtomicCmpXchgInst>(I) ) {
      // operands 2 and 3, whatever 1st class values
    } else if ( isa<AtomicRMWInst>(I) ) {
      // operand 3, whatever 1st class value
    }

  } // end op loop
} // end instrumentOperands

/*Instruction* AFLCoverage::instrumentInstruction(Instruction *I, GlobalVariable *GFZMapPtr,
                                GlobalVariable *GFZRandPtr,
                                GlobalVariable *GFZRandIdx) {

  Function *F = I->getFunction();
  Module *M = F->getParent();
  LLVMContext &C = M->getContext();

  //IntegerType *Int8Ty = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  Type *IT = I->getType();
  Instruction *NI = I->getNextNode();

  // PHI must be grouped at the beginning of the basic block
  while (NI && isa<PHINode>(I))
      NI = NI->getNextNode();

  if (!NI)
      return NULL;

  // use an id to reference the fuzzable instruction in gfz map
  ConstantInt *InstID = ConstantInt::get(Int32Ty, inst_id);

  IRBuilder<> IRB(NI);
  UnreachableInst *FakeVal = IRB.CreateUnreachable();
  I->replaceAllUsesWith(FakeVal);

  LoadInst *MapPtr = IRB.CreateLoad(GFZMapPtr);
  Value *MapPtrIdx = IRB.CreateGEP(MapPtr, InstID);
  LoadInst *InstStatus = IRB.CreateLoad(MapPtrIdx);

#ifdef branching_en
  Value *InstEnabled = IRB.CreateICmpEQ(InstStatus, ConstantInt::get(Int32Ty, 0));

  TerminatorInst *CheckStatus = SplitBlockAndInsertIfThen(InstEnabled, NI, false,
          MDBuilder(C).createBranchWeights((1<<20)-1, 1));

  IRB.SetInsertPoint(CheckStatus);
#endif

  // InstStatus is the status of the instruction, tells us if it
  // should be mutated or not

  PointerType *IPT = PointerType::getUnqual(I->getType());

  LoadInst *RandIdx = IRB.CreateLoad(GFZRandIdx);
  LoadInst *RandPtr = IRB.CreateLoad(GFZRandPtr);

  Value *RandPtrCast = IRB.CreateZExtOrBitCast(RandPtr, IPT);
  Value *RandPtrIdx = IRB.CreateGEP(RandPtrCast, RandIdx);

  // access random pool

  // inc idx to access rand pool
  Value *Incr = IRB.CreateAdd(RandIdx, ConstantInt::get(Int32Ty, 1));
  Incr = IRB.CreateURem(Incr, ConstantInt::get(Int32Ty, 4096));
  IRB.CreateStore(Incr, GFZRandIdx);

  LoadInst *RandVal = IRB.CreateLoad(RandPtrIdx);


  // hopefully should be right, trick to clear negative numbers without
  // mul once fuzzed val is off, it stays so
  Value *SubInst =
      IRB.CreateSub(InstStatus, ConstantInt::get(Int32Ty, -1));
  Value *MShift = ConstantInt::get(Int32Ty, 7);
  Value *Mask = IRB.CreateLShr(SubInst, MShift);
  Value *Subs = IRB.CreateSub(Mask, ConstantInt::get(Int32Ty, -1));
  Value *SubStat = IRB.CreateAnd(InstStatus, Subs);

  IRB.CreateStore(SubStat, MapPtrIdx);

  Value *FuzzVal;

  if (IT->isFloatingPointTy()) {
    FuzzVal = IRB.CreateSIToFP(InstStatus, IT);
    FuzzVal = IRB.CreateMul(FuzzVal, RandVal);
  } else {
    if (IT->getPrimitiveSizeInBits() <
        InstStatus->getType()->getPrimitiveSizeInBits()) {
      FuzzVal = IRB.CreateAnd(IRB.CreateTrunc(InstStatus, IT), RandVal);
    } else {
      FuzzVal =
          IRB.CreateAnd(IRB.CreateSExtOrBitCast(InstStatus, IT), RandVal);
    }
  }

  Value *FuzzedVal = IRB.CreateAdd(FuzzVal, I);

  //dyn_cast<Instruction>(FuzzedVal);

#ifdef branching_en
  IRB.SetInsertPoint(NI);

  PHINode *PHI = IRB.CreatePHI(IT, 2);
  BasicBlock *CondBlock = I->getParent();
  PHI->addIncoming(I, CondBlock);
  BasicBlock *ThenBlock = cast<Instruction>(FuzzedVal)->getParent();
  PHI->addIncoming(FuzzedVal, ThenBlock);

  FakeVal->replaceAllUsesWith(PHI);
#else
  FakeVal->replaceAllUsesWith(FuzzedVal);
#endif
  FakeVal->eraseFromParent();

  inst_id++;
  inst_inst++;

  return NI;
}*/

bool AFLCoverage::runOnModule(Module &M) {

  // File used for debugging  
  map_key_fd = fopen("./map_key.txt", "a");

  LLVMContext &C = M.getContext();

  IntegerType *Int8Ty = IntegerType::getInt8Ty(C);
  IntegerType *Int16Ty = IntegerType::getInt16Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);
  
  GlobalVariable *GFZMapPtr =
      new GlobalVariable(M, PointerType::get(Int16Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__gfz_map_area");

  GlobalVariable *GFZRandPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__gfz_rand_area");

  GlobalVariable *GFZRandIdx = new GlobalVariable(
      M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__gfz_rand_idx");

  StringSet<> WhitelistSet;
  SmallVector<Instruction*, 64> toInstrumentOperands, toInstrumentResult;

  // Show a banner

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {
    SAYF(cCYA "gfz-llvm-pass " cBRI VERSION cRST
              " by <lszekeres@google.com>, <_ocean>\n");
  } else {
    be_quiet = 1;
  }

  // Get temporary file to keep ID of fuzzed register to be used as gfz_map idx
  
  struct flock lock;
  int idfd;

  idfd = open(IDTMPFILE, O_CREAT | O_RDWR | O_NOATIME,
              S_IRUSR | S_IWUSR | S_IRGRP);
  memset(&lock, 0, sizeof(lock));

  // Whitelist functions
  
  char *whitelist = getenv("GFZ_WHITE_LIST");

  // Scan for functions explicitly marked as fuzzable
  
  for (auto &F : M) if (F.getSection() == ".fuzzables")
      WhitelistSet.insert(F.getName());

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

  int inst_fun = 0;
  
  int mod_tot = 0;
  int mod_sel = 0;
  int fun_tot = 0;
  int fun_sel = 0;

  // TODO: implement skipping of functions and modules by implementing
  // a #pragma nofuzz and __attribute(nofuzz)

  // ugly hack, don't want to implement an LTO pass,
  // keep track of current ID in a locked file
  lock.l_type = F_WRLCK;
  fcntl(idfd, F_SETLKW, &lock);
  lseek(idfd, 0, SEEK_SET);

  if (read(idfd, &inst_id, sizeof(inst_id)) != sizeof(inst_id))
    inst_id = 0;

  if (read(idfd, &tot, sizeof(tot)) != sizeof(tot))
    tot = 0;

  if (read(idfd, &sel, sizeof(sel)) != sizeof(sel))
    sel = 0;

  SAYF("\n=== Module %s\n", M.getName().str().c_str());

  for (auto &F : M) { // function loop
    
    fun_tot = 0;
    fun_sel = 0;

    if (whitelist && (WhitelistSet.find(F.getName()) == WhitelistSet.end())) {
      continue;
    }

    if (F.isIntrinsic()) {
      continue;
    }

    for (auto &BB : F) { // basic block loop

      if (BB.getFirstInsertionPt() == BB.end()) {
        continue;
      }

      for (auto &I : BB) { // instruction loop

        fun_tot++;

        // 'select': only instrument result
        if ( isa<SelectInst>(I) || isa<CmpInst>(I) ) {
          toInstrumentResult.push_back(&I);
          fun_sel++;
          continue;
        }

        // instrument blacklist
        if ( isa<BranchInst>(I) || isa<SwitchInst>(I) ||
             isa<IndirectBrInst>(I) || isa<CatchSwitchInst>(I) ||
             isa<CatchReturnInst>(I) || isa<CleanupReturnInst>(I) ||
             isa<UnreachableInst>(I) || isa<ShuffleVectorInst>(I) ||
             isa<AllocaInst>(I) || isa<FenceInst>(I) ||
             isa<CastInst>(I) || isa<PHINode>(I) ||
             isa<VAArgInst>(I) || isa<LandingPadInst>(I) ||
             isa<CatchPadInst>(I) || isa<CleanupPadInst>(I) ||
             isa<IntrinsicInst>(I) ) {
          continue;
        }

        toInstrumentOperands.push_back(&I);
        fun_sel++;

        // instrument result blacklist
        if ( isa<ReturnInst>(I) || isa<ResumeInst>(I) ||
             isa<InsertElementInst>(I) || isa<AtomicCmpXchgInst>(I) ||
             isa<AtomicRMWInst>(I) || isa<LoadInst>(I) ||
             isa<StoreInst>(I) || isa<GetElementPtrInst>(I) ) {
          continue;
        }

        Type *IT = I.getType();

        if (! (IT->isIntegerTy() || IT->isPointerTy() /*|| IT->isFloatingPointTy()*/) ) {
          continue;
        }

        toInstrumentResult.push_back(&I);

      } // end instruction loop
    } // end basic block loop

    if (fun_sel) {
      inst_fun++;
      
      if (!be_quiet) {
        OKF("Function %s: selected %u/%u instructions.", F.getName().str().c_str(), fun_sel, fun_tot);
      }
    } else {
      if (!be_quiet) {
        OKF("Function %s: skipped.", F.getName().str().c_str());
      }
    }
  
    mod_tot += fun_tot;
    mod_sel += fun_sel;

  } // end function loop

  SAYF("\n");
  OKF("Module %s: selected %u/%u instructions in %u function(s).", M.getName().str().c_str(), mod_sel, mod_tot, inst_fun);

  tot += mod_tot;
  sel += mod_sel;

  OKF("Total: selected %u/%u instructions.", sel, tot);

  // Pass the selected instructions to the 
  // corresponding instrumenting functions

  /* TODO: fix/implement this
  for (auto I: toInstrumentResult)
    instrumentResult(I, GFZMapPtr, GFZRandPtr, GFZRandIdx);
  */

  for (auto I: toInstrumentOperands)
    instrumentOperands(I, GFZMapPtr, GFZRandPtr, GFZRandIdx);

  OKF("Total: instrumented %u locations.", inst_id);

  // Write info that needs to be maintained between modules to IDTMPFILE

  lseek(idfd, 0, SEEK_SET);
  if (write(idfd, &inst_id, sizeof(inst_id)) != sizeof(inst_id)) {
    FATAL("Got a problem while writing current instruction id on %s", IDTMPFILE);
  }
  if (write(idfd, &tot, sizeof(tot)) != sizeof(tot)) {
    FATAL("Got a problem while writing total instructions on %s", IDTMPFILE);
  }
  if (write(idfd, &sel, sizeof(sel)) != sizeof(sel)) {
    FATAL("Got a problem while writing selected instructions on %s", IDTMPFILE);
  }
  lock.l_type = F_UNLCK;
  fcntl(idfd, F_SETLKW, &lock);

  if (!be_quiet && !inst_fun) {
    WARNF("No instrumentation targets found.");
    // there's functions with 0 I/ 0 BB (external references?) check them out....
  }

  fclose(map_key_fd);

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