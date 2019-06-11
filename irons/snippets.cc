/*
  Pointer instrumentation: work with Int64(s), just
  ptrtoint the operand before performing math, and then
  inttoptr it back at the end.

  Downside of this: segfaults like no tomorrow.
  Probably need to act on the pointed buffer.
*/

MutationFlags = IRB.CreateZExt(MutationFlags, IntegerType::getInt64Ty(C));

// Op * (MutationFlags & 00000001)
Value *KeepOriginal = IRB.CreateMul(
  IRB.CreatePtrToInt(Op, IntegerType::getInt64Ty(C)),
  IRB.CreateAnd(MutationFlags,
                ConstantInt::get(IntegerType::getInt64Ty(C), 1)));

// 1 * ((MutationFlags & 00000010) >> 1)
Value *PlusOne = IRB.CreateMul(
  ConstantInt::get(IntegerType::getInt64Ty(C), 1),
  IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                               ConstantInt::get(IntegerType::getInt64Ty(C), 2)),
                 ConstantInt::get(IntegerType::getInt64Ty(C), 1)));

// -1 * ((MutationFlags & 00000100) >> 2)
Value *MinusOne = IRB.CreateMul(
  ConstantInt::get(IntegerType::getInt64Ty(C), -1),
  IRB.CreateLShr(IRB.CreateAnd(MutationFlags,
                               ConstantInt::get(IntegerType::getInt64Ty(C), 4)),
                 ConstantInt::get(IntegerType::getInt64Ty(C), 2)));

// Add mutations together to obtain the new operand
FuzzedVal = IRB.CreateAdd(KeepOriginal,
              IRB.CreateAdd(PlusOne, MinusOne));

FuzzedVal = IRB.CreateIntToPtr(FuzzedVal, OT);

/*
  Increase idx to access random pool
  
*/
Value *Incr = IRB.CreateAdd(RandIdx, ConstantInt::get(Int32Ty, 1));
Incr = IRB.CreateURem(Incr, ConstantInt::get(Int32Ty, 4096));
IRB.CreateStore(Incr, GFZRandIdx);

/*
  Use the print method that works with raw_ostream &OS.

*/

std::string str;
raw_string_ostream rso(str);
YOUR_INSTANCE->print(rso);
std::cout << rso.str();

/*
  This is useless because we can just check for

    isa<IntrinsicInst>(I)
*/

// do not instrument calls to intrinsics
if ( isa<CallInst>(I) ) {
  Function *fun = cast<CallInst>(I).getCalledFunction();
  if (fun && fun->isIntrinsic()) {
    printf("\n[*] call to intrinsic, skipping");
    continue;
  }
}

// do not instrument invokes to intrinsics
if ( isa<InvokeInst>(I) ) {
  Function *fun = cast<InvokeInst>(I).getCalledFunction();
  if (fun && fun->isIntrinsic()) {
    printf("\n[*] invoke intrinsic, skipping");
    continue;
  }
}

/*
  Snippet for adapting the size of the value to add to the
  original operand.

  IRB should do it anyway though, that's what I've seen.
*/

Value *FuzzVal;

if (OIT->isFloatingPointTy()) {
  FuzzVal = IRB.CreateSIToFP(InstStatus, IT);
  FuzzVal = IRB.CreateMul(FuzzVal, RandVal);
} else {
  if (OIT->getPrimitiveSizeInBits() <
      InstStatus->getType()->getPrimitiveSizeInBits()) {
    FuzzVal = IRB.CreateAnd(IRB.CreateTrunc(InstStatus, OIT), RandVal);
  } else {
    FuzzVal =
        IRB.CreateAnd(IRB.CreateSExtOrBitCast(InstStatus, OIT), RandVal);
  }
}

Value *FuzzedVal = IRB.CreateAdd(FuzzVal, OpI);

/*
  Snippet for decreasing ( ? ) the value in __gfz_map_area
  without ever going into the negative numbers.

  Dubbi.

  Anyway it's useless now since we act on __gfz_map_area
  "from the outside" i.e. from the parent process.

*/

// hopefully should be right, trick to clear negative numbers without
// mul once fuzzed val is off, it stays so
Value *SubInst = IRB.CreateSub(InstStatus, ConstantInt::get(Int32Ty, -1));
Value *MShift = ConstantInt::get(Int32Ty, 7);
Value *Mask = IRB.CreateLShr(SubInst, MShift);
Value *Subs = IRB.CreateSub(Mask, ConstantInt::get(Int32Ty, -1));
Value *SubStat = IRB.CreateAnd(InstStatus, Subs);

IRB.CreateStore(SubStat, MapPtrIdx);