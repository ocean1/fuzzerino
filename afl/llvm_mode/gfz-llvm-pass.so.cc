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
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/XRay/YAMLXRayRecord.h"
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
  uint32_t inst_id = 0;
  Value *fuzzInt(IRBuilder<> IRB, Instruction *I);
};

} // namespace

char AFLCoverage::ID = 0;

/// shamelessly stolen from lib/Transforms/Scalar/ConstantHoisting.cpp
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

Value *AFLCoverage::fuzzInt(IRBuilder<> IRB, Instruction *I) {

  Function *F = I->getFunction();
  Module *M = I->getModule();
  LLVMContext &C = F->getContext();
  IntegerType *Int8Ty = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);
  GlobalVariable *GFZMapPtr = M->getGlobalVariable("__gfz_map_area");
  GlobalVariable *GFZRandPtr = M->getGlobalVariable("__gfz_map_area");
  GlobalVariable *GFZRandIdx = M->getGlobalVariable("__gfz_rand_idx");

  Type *IT = I->getType();
  // use an id to reference the fuzzable instruction in gfz map
  ConstantInt *InstID = ConstantInt::get(Int32Ty, inst_id);

  LoadInst *MapPtr = IRB.CreateLoad(GFZMapPtr);
  MapPtr->setMetadata(M->getMDKindID("nosanitize"), MDNode::get(C, None));
  Value *MapPtrIdx = IRB.CreateGEP(MapPtr, InstID);

  /* InstStatus is the status of the instruction, tells us if it
   * should be mutated or not */

  PointerType *IPT = PointerType::getUnqual(IT);

  LoadInst *RandIdx = IRB.CreateLoad(GFZRandIdx);
  LoadInst *RandPtr = IRB.CreateLoad(GFZRandPtr);
  RandPtr->setMetadata(M->getMDKindID("nosanitize"), MDNode::get(C, None));

  Value *RandPtrCast = IRB.CreateZExtOrBitCast(RandPtr, IPT);
  Value *RandPtrIdx = IRB.CreateGEP(RandPtrCast, RandIdx);

  /* access random pool */

  /* inc idx to access rand pool */
  Value *Incr = IRB.CreateAdd(RandIdx, ConstantInt::get(Int32Ty, 1));
  Incr = IRB.CreateURem(Incr, ConstantInt::get(Int32Ty, 4096));
  IRB.CreateStore(Incr, GFZRandIdx)
      ->setMetadata(M->getMDKindID("nosanitize"), MDNode::get(C, None));

  LoadInst *RandVal = IRB.CreateLoad(RandPtrIdx);

  LoadInst *InstStatus = IRB.CreateLoad(MapPtrIdx);

  // hopefully should be right, trick to clear negative numbers without mul
  // once fuzzed val is off, it stays so
  Value *SubInst = IRB.CreateSub(InstStatus, ConstantInt::get(Int8Ty, -1));
  Value *MShift = ConstantInt::get(Int8Ty, 7);
  Value *Mask = IRB.CreateLShr(SubInst, MShift);
  Value *Subs = IRB.CreateSub(Mask, ConstantInt::get(Int8Ty, -1));
  Value *SubStat = IRB.CreateAnd(InstStatus, Subs);

  IRB.CreateStore(SubStat, MapPtrIdx)
      ->setMetadata(M->getMDKindID("nosanitize"), MDNode::get(C, None));

  Value *FuzzVal;

  if (IT->isFloatingPointTy()) {
    FuzzVal = IRB.CreateSIToFP(InstStatus, IT);
    FuzzVal = IRB.CreateAnd(FuzzVal, RandVal);
  } else {
    if (IT->getPrimitiveSizeInBits() <
        InstStatus->getType()->getPrimitiveSizeInBits()) {
      FuzzVal = IRB.CreateAnd(IRB.CreateTrunc(InstStatus, IT), RandVal);
    } else {
      FuzzVal = IRB.CreateAnd(IRB.CreateSExtOrBitCast(InstStatus, IT), RandVal);
    }
  }

  Value *FuzzedVal = IRB.CreateAdd(FuzzVal, I);

  dyn_cast<Instruction>(FuzzedVal)->setMetadata(M->getMDKindID("nosanitize"),
                                                MDNode::get(C, None));

  return FuzzVal;
}

bool AFLCoverage::runOnModule(Module &M) {

  LLVMContext &C = M.getContext();

  IntegerType *Int8Ty = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);
  GlobalVariable *GFZMapPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__gfz_map_area");

  GlobalVariable *GFZRandPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__gfz_rand_area");

  GlobalVariable *GFZRandIdx = new GlobalVariable(
      M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__gfz_rand_idx");

  StringSet<> WhitelistSet;
  /* Show a banner */

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {

    SAYF(cCYA "gfz-llvm-pass " cBRI VERSION cRST
              " by <lszekeres@google.com>, <ocean>\n");

  } else
    be_quiet = 1;

  /* get temporary file to keep ID of fuzzed register to be used as gfz_map idx
   */
  struct flock lock;
  int idfd;

  idfd = open(IDTMPFILE, O_CREAT | O_RDWR | O_NOATIME,
              S_IRUSR | S_IWUSR | S_IRGRP);
  memset(&lock, 0, sizeof(lock));

  /* Decide instrumentation ratio */

  char *inst_ratio_str = getenv("AFL_INST_RATIO");
  unsigned int inst_ratio = 100;

  if (inst_ratio_str) {

    if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
        inst_ratio > 100)
      FATAL("Bad value of AFL_INST_RATIO (must be between 1 and 100)");
  }

  /* white list functions */
  char *whitelist = getenv("GFZ_WHITE_LIST");

  /* scan for functions explicitly marked as fuzzable */
  for (auto &F : M) {
    if (F.getSection() == ".fuzzables") {
      WhitelistSet.insert(F.getName());
    }
  }

  if (whitelist) {
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

  /* global variables for GFZ */
  // 0, GlobalVariable::GeneralDynamicTLSModel, 0, false);

  /* Instrument all the things! */

  int inst_blocks = 0, inst_func = 0;
  unsigned int nope = 0;

  // TODO: implement skipping of functions and modules by implementing
  // a #pragma nofuzz and __attribute(nofuzz)

  // ugly hack, don't want to implement an LTO pass, keep track of current ID in
  // a locked file
  lock.l_type = F_WRLCK;
  fcntl(idfd, F_SETLKW, &lock);
  lseek(idfd, 0, SEEK_SET);
  if (read(idfd, &inst_id, sizeof(inst_id)) != sizeof(inst_id))
    inst_id = 0;

  SmallVector<Instruction *, 1000> RemoveInst;
  for (auto &F : M) {
    if (whitelist && (WhitelistSet.find(F.getName()) == WhitelistSet.end())) {
      continue;
    }

    if (F.getName().startswith("llvm") || F.getName().startswith("__llvm"))
      continue;

    OKF("Instrumenting %s", F.getName().str().c_str());
    inst_func++;

    for (auto &BB : F) {

      Instruction *NI = NULL;

      for (auto &I : BB) {

        // skip over inserted instructions
        if (NI != NULL && &I != NI)
          continue;

        /* fuzz also all operands */

        User::value_op_iterator OI, OE;

        /* heuristics to skip functions that might crash the target
         * too easily, will implement some better heuristics later :) */

        if (
            // dyn_cast<StoreInst>(&I)         ||
            // dyn_cast<CmpInst>(&I)           ||
            dyn_cast<AllocaInst>(&I) || dyn_cast<BranchInst>(&I) ||
            dyn_cast<SwitchInst>(&I) || dyn_cast<InvokeInst>(&I) ||
            dyn_cast<IndirectBrInst>(&I) || dyn_cast<UnreachableInst>(&I) ||
            // dyn_cast<GetElementPtrInst>(&I) ||
            // dyn_cast<ReturnInst>(&I)        ||
            isa<PHINode>(&I) // PHY will need special handling
        ) {
          continue;
        }

        for (OI = I.value_op_begin(), OE = I.value_op_end(); OI != OE; ++OI) {
          // for each op, if type is Int (will need to add also for floats, and
          // other types....) take OI as Value, push fuzzed val, substitute it
          // val = insertFuzzVal(I, OI);
          // updateOp(OI, val);
          // Value *inval = dyn_cast<Value*>(OI);
          auto *OpI = dyn_cast<Instruction>(*OI);

          if (!OpI)
            continue;

          // Value* fuzzed = doFuzzVal(OpI, ...);
          // updateOperand(fuzzed, I, OI.num);
        }

        I.setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

        Type *IT = I.getType();
        if (!(IT->isIntegerTy() /*|| IT->isFloatingPointTy()*/) ||
            IT->isPointerTy()) {
          continue;
        }

        NI = (&I)->getNextNode();
        IRBuilder<> IRB(NI);
        // use an id to reference the fuzzable instruction in gfz map
        ConstantInt *InstID = ConstantInt::get(Int32Ty, inst_id);

        LoadInst *MapPtr = IRB.CreateLoad(GFZMapPtr);
        MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
        Value *MapPtrIdx = IRB.CreateGEP(MapPtr, InstID);

        /* InstStatus is the status of the instruction, tells us if it
         * should be mutated or not */

        PointerType *IPT = PointerType::getUnqual(I.getType());

        LoadInst *RandIdx = IRB.CreateLoad(GFZRandIdx);
        LoadInst *RandPtr = IRB.CreateLoad(GFZRandPtr);
        RandPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

        Value *RandPtrCast = IRB.CreateZExtOrBitCast(RandPtr, IPT);
        Value *RandPtrIdx = IRB.CreateGEP(RandPtrCast, RandIdx);

        /* access random pool */

        /* inc idx to access rand pool */
        Value *Incr = IRB.CreateAdd(RandIdx, ConstantInt::get(Int32Ty, 1));
        Incr = IRB.CreateURem(Incr, ConstantInt::get(Int32Ty, 4096));
        IRB.CreateStore(Incr, GFZRandIdx)
            ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

        LoadInst *RandVal = IRB.CreateLoad(RandPtrIdx);

        LoadInst *InstStatus = IRB.CreateLoad(MapPtrIdx);

        // hopefully should be right, trick to clear negative numbers without
        // mul once fuzzed val is off, it stays so
        Value *SubInst =
            IRB.CreateSub(InstStatus, ConstantInt::get(Int8Ty, -1));
        Value *MShift = ConstantInt::get(Int8Ty, 7);
        Value *Mask = IRB.CreateLShr(SubInst, MShift);
        Value *Subs = IRB.CreateSub(Mask, ConstantInt::get(Int8Ty, -1));
        Value *SubStat = IRB.CreateAnd(InstStatus, Subs);

        IRB.CreateStore(SubStat, MapPtrIdx)
            ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

        Value *FuzzVal;

        if (IT->isFloatingPointTy()) {
          FuzzVal = IRB.CreateSIToFP(InstStatus, IT);
          FuzzVal = IRB.CreateAnd(FuzzVal, RandVal);
        } else {
          if (IT->getPrimitiveSizeInBits() <
              InstStatus->getType()->getPrimitiveSizeInBits()) {
            FuzzVal = IRB.CreateAnd(IRB.CreateTrunc(InstStatus, IT), RandVal);
          } else {
            FuzzVal =
                IRB.CreateAnd(IRB.CreateSExtOrBitCast(InstStatus, IT), RandVal);
          }
        }

        Value *FuzzedVal = IRB.CreateAdd(FuzzVal, &I);

        dyn_cast<Instruction>(FuzzedVal)->setMetadata(
            M.getMDKindID("nosanitize"), MDNode::get(C, None));

        I.replaceAllUsesWith(FuzzedVal);

        // replace back I as operand of fuzzedVal, got removed by
        // replaceAllUses
        updateOperand(dyn_cast<Instruction>(FuzzedVal), 1, &I);

        // RemoveInst.push_back(&I);
        nope++;

        inst_id++;
      }

      inst_blocks++;
    }
  }

ciccio:
  while (!RemoveInst.empty()) {
    Instruction *RI = RemoveInst.pop_back_val();
    // TODO: find how to remove these... keeps on popping at random...
    RI->removeFromParent();
    if (nope == 0)
      nope = 50;
  }

  OKF("Removing stuff %u", nope);

  lseek(idfd, 0, SEEK_SET);
  if (write(idfd, &inst_id, sizeof(inst_id)) != sizeof(inst_id)) {
    FATAL("uh oh got a problem while writing current instruction id on %s",
          IDTMPFILE);
  }
  lock.l_type = F_UNLCK;
  fcntl(idfd, F_SETLKW, &lock);

  /* Say something nice. */

  if (!be_quiet) {

    if (!inst_blocks)
      WARNF("No instrumentation targets found.");
    else
      OKF("Instrumented %u BB, %u I, %u F (%s mode, ratio %u%%).", inst_blocks,
          inst_id, inst_func,
          getenv("AFL_HARDEN")
              ? "hardened"
              : ((getenv("AFL_USE_ASAN") || getenv("AFL_USE_MSAN"))
                     ? "ASAN/MSAN"
                     : "non-hardened"),
          inst_ratio);
  }

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
