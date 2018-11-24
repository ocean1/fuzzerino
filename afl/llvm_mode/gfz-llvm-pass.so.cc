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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"

using namespace llvm;

namespace {

  class AFLCoverage : public ModulePass {

    public:

      static char ID;
      AFLCoverage() : ModulePass(ID) { }

      bool runOnModule(Module &M) override;

      // StringRef getPassName() const override {
      //  return "American Fuzzy Lop Instrumentation";
      // }

  };

}


char AFLCoverage::ID = 0;


bool AFLCoverage::runOnModule(Module &M) {

  LLVMContext &C = M.getContext();

  IntegerType *Int8Ty  = IntegerType::getInt8Ty(C);
  IntegerType *Int32Ty = IntegerType::getInt32Ty(C);

  /* Show a banner */

  char be_quiet = 0;

  if (isatty(2) && !getenv("AFL_QUIET")) {

    SAYF(cCYA "gfz-llvm-pass " cBRI VERSION cRST " by <lszekeres@google.com>, <ocean>\n");

  } else be_quiet = 1;

  /* get temporary file to keep ID of fuzzed register to be used as gfz_map idx */
  struct flock lock;
  int idfd;

  idfd = open(IDTMPFILE, O_CREAT | O_RDWR | O_NOATIME, S_IRUSR | S_IWUSR | S_IRGRP );
  memset (&lock, 0, sizeof(lock));

  /* Decide instrumentation ratio */

  char* inst_ratio_str = getenv("AFL_INST_RATIO");
  unsigned int inst_ratio = 100;

  if (inst_ratio_str) {

    if (sscanf(inst_ratio_str, "%u", &inst_ratio) != 1 || !inst_ratio ||
        inst_ratio > 100)
      FATAL("Bad value of AFL_INST_RATIO (must be between 1 and 100)");

  }

  /* global variables for GFZ */
  GlobalVariable *GFZMapPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__gfz_map_area");

  GlobalVariable *GFZRandPtr =
      new GlobalVariable(M, PointerType::get(Int8Ty, 0), false,
                         GlobalValue::ExternalLinkage, 0, "__gfz_rand_area");

  GlobalVariable *GFZRandIdx = new GlobalVariable(
      M, Int32Ty, false, GlobalValue::ExternalLinkage, 0, "__gfz_rand_idx");
      //0, GlobalVariable::GeneralDynamicTLSModel, 0, false);

  /* Instrument all the things! */

  int inst_blocks = 0, inst_inst = 0, inst_func = 0;

  // TODO: implement skipping of functions and modules by implementing
  // a #pragma nofuzz and __attribute(nofuzz)

  // ugly hack, don't want to implement an LTO pass, keep track of current ID in a locked file
  lock.l_type = F_WRLCK;
  fcntl(idfd, F_SETLKW, &lock);
  lseek(idfd, 0, SEEK_SET);
  if (read(idfd, &inst_inst, sizeof(inst_inst)) != sizeof(inst_inst))
    inst_inst = 0;

  for (auto &F : M) {
    if (F.getName().startswith("llvm.")) {
    //  || F.getSection() != ".fuzzables") {
      continue;
    }
    //OKF("Instrumenting %s", F.getName().str().c_str());

    for (auto &BB : F) {
        inst_func++;

      for (auto &I: BB) {

        /* heuristics to skip functions that might crash the target
         * too easily, will implement some better heuristics later :) */

        if (dyn_cast<StoreInst>(&I)         ||
            dyn_cast<CmpInst>(&I)           ||
            dyn_cast<CallInst>(&I)          ||
            dyn_cast<AllocaInst>(&I)        ||
            dyn_cast<BranchInst>(&I)        ||
            dyn_cast<ReturnInst>(&I)        ||
            dyn_cast<SwitchInst>(&I)        ||
            dyn_cast<InvokeInst>(&I)        ||
            dyn_cast<ResumeInst>(&I)        ||
            dyn_cast<IndirectBrInst>(&I)    ||
            dyn_cast<UnreachableInst>(&I)   ||
            dyn_cast<GetElementPtrInst>(&I) ||
            isa<PHINode>(&I)
            ){
          continue;
        }


        Type *t = I.getType();
        if (!(t->isIntegerTy() || t->isFloatingPointTy())) {
            continue;
        }

        IRBuilder<> IRB(&I);
        // use an id to reference the fuzzable instruction in gfz map
        ConstantInt *InstID = ConstantInt::get(Int32Ty, inst_inst);

        LoadInst *MapPtr = IRB.CreateLoad(GFZMapPtr);
        MapPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
        Value *MapPtrIdx =
            IRB.CreateGEP(MapPtr, InstID);

        /* InstStatus is the status of the instruction, tells us if it
         * should be mutated or not */
        LoadInst *InstStatus = IRB.CreateLoad(MapPtrIdx);

        LoadInst *RandIdx = IRB.CreateLoad(GFZRandIdx);
        LoadInst *RandPtr = IRB.CreateLoad(GFZRandPtr);
        RandPtr->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));
        Value *RandPtrIdx =
            IRB.CreateGEP(RandPtr, RandIdx);

        /* access random pool */
        //LoadInst *RandVal = IRB.CreateLoad(RandPtrIdx);

        /* inc idx to access rand pool */
        Value *Incr = IRB.CreateAdd(RandIdx, ConstantInt::get(Int8Ty, 1));
        IRB.CreateStore(Incr, GFZRandIdx)
            ->setMetadata(M.getMDKindID("nosanitize"), MDNode::get(C, None));

        auto *NI = (&I)->clone();

        IRB.Insert(NI);

        Value *FuzzVal = IRB.CreateZExt(
                IRB.CreateAnd(RandPtrIdx, InstStatus),
                I.getType());

        Value *FuzzedVal = IRB.CreateXor(FuzzVal, NI);

        I.replaceAllUsesWith(FuzzedVal);
        //I.eraseFromParent();

        inst_inst++;
      }

      inst_blocks++;
    }

  }

  lseek(idfd, 0, SEEK_SET);
  if( write(idfd, &inst_inst, sizeof(inst_inst)) != sizeof(inst_inst)){
    FATAL("uh oh got a problem while writing current instruction id on %s", IDTMPFILE);
  }
  lock.l_type = F_UNLCK;
  fcntl (idfd, F_SETLKW, &lock);
  /* Say something nice. */

  if (!be_quiet) {

    if (!inst_blocks) WARNF("No instrumentation targets found.");
    else OKF("Instrumented %u BB, %u I, %u F (%s mode, ratio %u%%).",
             inst_blocks, inst_inst, inst_func, getenv("AFL_HARDEN") ? "hardened" :
             ((getenv("AFL_USE_ASAN") || getenv("AFL_USE_MSAN")) ?
              "ASAN/MSAN" : "non-hardened"), inst_ratio);

  }

  return true;

}


static void registerAFLPass(const PassManagerBuilder &,
                            legacy::PassManagerBase &PM) {

  PM.add(new AFLCoverage());

}


static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_OptimizerLast, registerAFLPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerAFLPass);
