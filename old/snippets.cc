/* afl-cmin attempt: sucks */

void afl_cmin() {

  int i = 0;
  u8 *cmd;
  u8 *min_dirs[GFZ_MAX_MIN_TARGETS];

  for (i = 0; i < num_min_targets; ++i) {

    min_dirs[i] = alloc_printf("%s_min_%d", gen_dir, i);

    // Execute afl-cmin

    cmd = alloc_printf("env -i AFL_PATH=%s %s/afl-cmin -i %s -o %s -- %s >> /dev/null 2>> /dev/null",
                       getenv("AFL_PATH"), getenv("AFL_PATH"), gen_dir, min_dirs[i], min_targets[i]);
    system(cmd);
    ck_free(cmd);

  }

  // Delete generated dir

  cmd = alloc_printf("rm -rf %s", gen_dir);
  system(cmd);
  ck_free(cmd);

  // Rename first min dir to generated dir

  rename(min_dirs[0], gen_dir);
  
  if (num_min_targets > 1) {

    // Merge minimized folders

    for (i = 1; i < num_min_targets; ++i) {

      cmd = alloc_printf("rsync -a %s/ %s/", min_dirs[i], gen_dir);
      system(cmd);
      ck_free(cmd);

    }

  }

  // Cleanup

  ck_free(min_dirs[0]);

  for (i = 1; i < num_min_targets; ++i) {

    cmd = alloc_printf("rm -rf %s", min_dirs[i]);
    system(cmd);
    ck_free(cmd);
    ck_free(min_dirs[i]);

  }

  // Count number of unique seeds

  struct dirent *dp;
  DIR *dfd = opendir(gen_dir);

  u64 count = 0;
  
  while (( dp = readdir(dfd) ))
    if ( dp->d_type != DT_DIR )
      ++count;

  closedir(dfd);

  if (count > gen_unique)
    last_seed_time = get_cur_time();

  gen_unique = count;
  gen_after_cmin = 0;

}

/* Dry run ban attempt: sucks */

if ( avg_exec < 100 ) {
  if (!slow_since) {
    slow_since = get_cur_time();
  } else if ( get_cur_time() > (slow_since + (GFZ_TMOUT_SEC * 1000)) ) {
    //fprintf(log_file, "\nbanning branch %d. slow_since = %llu, cur time adjusted = %llu", branch, (u64)slow_since/1000, (u64)get_cur_time()/1000);
    slow_since = 0;
    if (branch > -1) {
      __gfz_bnc_ban_map[branch] = 1;
      __gfz_bnc_ban_locs++;
    }
    __gfz_num_map[loc] = GFZ_KEEP_ORIGINAL;
    __gfz_num_active--;
    goto skip_branch;
  }
} else {
  slow_since = 0;
}

/* BEGIN debugging stuff */

fprintf(map_key_fd, "\n%s %d:\t%s", is_pointer ? "(ptr)" : "(num)", (is_pointer ? ptr_locs : num_locs) - 1, I->getOpcodeName());

//this sometimes crashes for nullptr dereference...
//if ( isa<CallInst>(I) )
//  fprintf(map_key_fd, " (%s)", dyn_cast<CallInst>(I)->getCalledFunction()->getName().str().c_str());

std::string str;
raw_string_ostream rso(str);
IT->print(rso);
fprintf(map_key_fd, ", type %s", rso.str().c_str());

/* END debugging stuff */

/* The generation is consistently slow... */

if ( avg_exec < 100 ) {
  if (!slow_since) {
    slow_since = get_cur_time();
  } else if ( get_cur_time() > (slow_since + (GFZ_DRY_TMOUT_SEC * 1000)) ) {
    // Go on to next location
    slow_since = 0;
    break;
  }
} else {
  slow_since = 0;
}

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

/* Increase idx to access random pool */
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

/* ban map dump for debugging... */

fprintf(log_file, "\ndry run tmouts: %d", total_tmouts);
fprintf(log_file, "\ndry run crashes: %d", total_crashes);

FILE *map_file;

if (!(map_file = fopen("./gfz_ban.map", "wb")))
{
    printf("Error! Could not open file\n");
    return -1;
}

ssize_t n_bytes = fwrite(__gfz_ban_ptr, __gfz_num_locs * sizeof(u16), 1, map_file);

if (n_bytes <= 0) {
    printf("ko (%ld)\n", n_bytes);
    return -1;
}
else
    printf("\n[+] Map written.\n");