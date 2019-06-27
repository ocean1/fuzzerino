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
Value *SubInst = IRB.CreateSub(MutationFlags, ConstantInt::get(Int32Ty, -1));
Value *MShift = ConstantInt::get(Int32Ty, 7);
Value *Mask = IRB.CreateLShr(SubInst, MShift);
Value *Subs = IRB.CreateSub(Mask, ConstantInt::get(Int32Ty, -1));
Value *SubStat = IRB.CreateAnd(MutationFlags, Subs);

IRB.CreateStore(SubStat, MapPtrIdx);

/* original queue gfz loop */

  while (i < maxi) {
      show_stats();
      u32 timeout = exec_tmout;
      
      fault = run_target(use_argv, timeout);

      if ( gen_file && fault == FAULT_NONE ) { // xxhash

        last_len = 0;
        last_md5 = md5(gen_file, &last_len);

        if ( new_output(last_md5) ) {

          // Add sample to front of queue
          struct gfz_q_entry *new = (struct gfz_q_entry*) malloc(sizeof(struct gfz_q_entry));
          new->len = last_len;
          new->md5 = last_md5;
          new->map = 0;
          new->next = gfz_q;

          gfz_q = new;

          // Save output in gen_dir
          u8 *fn = alloc_printf("%s/%d", gen_dir, i);
          // rename(gen_file, fn);
          ck_free(fn);

        } else {

          free(last_md5);
      
        }

      }

      switch (fault) {
        case FAULT_TMOUT:
          ++total_tmouts;
          break;
        case FAULT_CRASH:
          ++total_crashes;
          break;
      }

      if ( gfz_q ) {

      }

      ++i;

  }

/* Calculates the md5sum of a file given its file path. */

char *md5(const char *filename, u32 *len) {
  
  unsigned char c[MD5_DIGEST_LENGTH];
  int i;
  MD5_CTX mdContext;
  int bytes;
  unsigned char data[1024];
  char *filemd5 = (char*) malloc(33 *sizeof(char));

  FILE *inFile = fopen (filename, "rb");
  
  if (inFile == NULL) {
    perror(filename);
    return 0;
  }

  MD5_Init(&mdContext);

  while ((bytes = fread(data, 1, 1024, inFile)) != 0) {
    MD5_Update(&mdContext, data, bytes);
    
    if (len) {
      *len += bytes;
    }
  }
  
  MD5_Final(c, &mdContext);

  for(i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf(&filemd5[i*2], "%02x", (unsigned int)c[i]);
  }

  fclose(inFile);
  return filemd5;

}

/* Returns 0 if the md5 is found in the queue, 1 otherwise. */

int new_output(char *md5) {

  struct gfz_q_entry *q = gfz_q;

  while (q) {
    
    if ( !strncmp(md5, q->md5, MD5_DIGEST_LENGTH) )
      return 0;

    q = q->next;

  }

  return 1;

}

/* DEBUGGING STUFF */

void log_gfz_q() {

  struct gfz_q_entry *q = gfz_q;

  fprintf(log_file, "\ncurrent queue status:");

  if(!q) {
    fprintf(log_file, "\n    empty.");
    return;
  }

  while (q) {
    fprintf(log_file, "\n    len: %u, md5: %s", q->len, q->md5);
    q = q->next;
  }

}