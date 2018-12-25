/*
   american fuzzy lop - LLVM instrumentation bootstrap
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres.

   Copyright 2015, 2016 Google Inc. All rights reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

   This code is the rewrite of afl-as.h's main_payload.

*/

#include "../config.h"
#include "../debug.h"
#include "../types.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <malloc.h>

/* This is a somewhat ugly hack for the experimental 'trace-pc-guard' mode.
   Basically, we need to make sure that the forkserver is initialized after
   the LLVM-generated runtime initialization pass, not before. */

#ifdef USE_TRACE_PC
#define CONST_PRIO 5
#else
#define CONST_PRIO 0
#endif /* ^USE_TRACE_PC */

/* Globals needed by the injected instrumentation. The __afl_area_initial region
   is used for instrumentation output before __afl_map_shm() has a chance to
   run. It will end up as .comm, so it shouldn't be too wasteful. */

u8 __afl_area_initial[MAP_SIZE];
u8 *__afl_area_ptr = __afl_area_initial;

__thread u32 __afl_prev_loc;

/* "Rand" area gets filled by the thread itself every time we change fuzz
 * strategy or when all values have been used, for example filled with low/high
 * values, or random values */
u32 *__gfz_map_area = NULL;
u8 *__gfz_rand_area = NULL;
u32 __gfz_rand_idx;

/* keep 4MB of map, it's quite a lot of space, and our test targets
   will happily work with this, we can later optimize by storing inst_inst
   and merging global vars in a LTO pass (or brutally patched with lief) */
const ssize_t __gfz_map_size = 4 * 1024 * 1024;

/* Running in persistent mode? */

static u8 is_persistent;

/* SHM setup. */

// override free, generators are short lived programs (http://www.drdobbs.com/cpp/increasing-compiler-speed-by-over-75/240158941)
// https://twitter.com/johnregehr/status/1073801738707070977
// we could also override malloc to provide a little bit more space to reduce SEGFAULTs
void free(void *ptr)
{
}

static void __afl_map_shm(void) {

  u8 *id_str = getenv(SHM_ENV_VAR);

  /* If we're running under AFL, attach to the appropriate region, replacing the
     early-stage __afl_area_initial region that is needed to allow some really
     hacky .init code to work correctly in projects such as OpenSSL. */

  if (id_str) {

    u32 shm_id = atoi(id_str);

    __afl_area_ptr = shmat(shm_id, NULL, 0);

    /* Whooooops. */

    if (__afl_area_ptr == (void *)-1)
      _exit(1);

    /* Write something into the bitmap so that even with low AFL_INST_RATIO,
       our parent doesn't give up on us. */

    __afl_area_ptr[0] = 1;
  }
}

/* Fork server logic. */

static void __afl_start_forkserver(void) {

  static u8 tmp[4];
  s32 child_pid;

  //char *cmdfmt = "./outputs/%lu_fuzztest_%lu";
	//char *oldp = "/dev/shm/fuzztest";
  //char cmd[500];
  u8 child_stopped = 0;

  /* Phone home and tell the parent that we're OK. If parent isn't there,
     assume we're not running in forkserver mode and just execute program. */

  if (write(FORKSRV_FD + 1, tmp, 4) != 4) return;

  int i = -1; // make a clean dry run with one sample
  while(1) {
    u8 *inststat = getenv("GFZ_STAT_VAL");
    u8 insval = (inststat == NULL) ? 1 : atoi(inststat);
    u32 was_killed;
    int status;
    //int a, b;

    /* Wait for parent by reading from the pipe. Abort if read fails. */

    if (read(FORKSRV_FD, &was_killed, 4) != 4) _exit(1);

    // maybe we can have a socat listening and redirect there a specific
    // FD for input/output of the fuzzer and you can attach with a term
    // can this be done without breaking term? like redirect immediately
    // stdio, but using a select on it to wait for input? (asynchronous)

    /* If we stopped the child in persistent mode, but there was a race
       condition and afl-fuzz already issued SIGKILL, write off the old
       process. */

    if ((child_stopped)) { //&& was_killed) {
      child_stopped = 0;
      if (waitpid(child_pid, &status, 0) < 0)
        _exit(1);
    }

    if (!child_stopped) {

      /* Once woken up, create a clone of our process. */
      // a = (i+rand()) % maxi;
      // b = (i+rand()) % maxi;

	if (i > 0)
      __gfz_map_area[i] = insval;

      //__gfz_map_area[a] = insval;
      //__gfz_map_area[b] = insval;

      child_pid = fork();
      if (child_pid < 0)
        _exit(1);

      /* In child process: close fds, resume execution. */

      if (!child_pid) {

        close(FORKSRV_FD);
        close(FORKSRV_FD + 1);
        return;
      }

      __gfz_map_area[i] = 0;

      //sprintf(cmd, cmdfmt, (unsigned long)time(NULL), i);
      //(void)system(cmd);
      //rename(oldp, cmd);

      ++i;
      //__gfz_map_area[a] = 0;
      //__gfz_map_area[b] = 0;

    } else {
      /* Special handling for persistent mode: if the child is alive but
         currently stopped, simply restart it with SIGCONT. */

      kill(child_pid, SIGCONT);
      child_stopped = 0;
    }

    /* In parent process: write PID to pipe, then wait for child. */

    if (write(FORKSRV_FD + 1, &child_pid, 4) != 4) _exit(1);

    if (waitpid(child_pid, &status, is_persistent ? WUNTRACED : 0) < 0)
      _exit(1);

    /* In persistent mode, the child stops itself with SIGSTOP to indicate
       a successful run. In this case, we want to wake it up without forking
       again. */

    if (WIFSTOPPED(status))
      child_stopped = 1;

    /* Relay wait status to pipe, then loop back. */

    if (write(FORKSRV_FD + 1, &status, 4) != 4) _exit(1);

  }
}

/* A simplified persistent mode handler, used as explained in README.llvm. */

int __afl_persistent_loop(unsigned int max_cnt) {

  static u8 first_pass = 1;
  static u32 cycle_cnt;

  if (first_pass) {

    /* Make sure that every iteration of __AFL_LOOP() starts with a clean slate.
       On subsequent calls, the parent will take care of that, but on the first
       iteration, it's our job to erase any trace of whatever happened
       before the loop. */

    if (is_persistent) {

      memset(__afl_area_ptr, 0, MAP_SIZE);
      __afl_area_ptr[0] = 1;
      __afl_prev_loc = 0;
    }

    cycle_cnt = max_cnt;
    first_pass = 0;
    return 1;
  }

  if (is_persistent) {

    if (--cycle_cnt) {

      raise(SIGSTOP);

      __afl_area_ptr[0] = 1;
      __afl_prev_loc = 0;

      return 1;

    } else {

      /* When exiting __AFL_LOOP(), make sure that the subsequent code that
         follows the loop is not traced. We do that by pivoting back to the
         dummy output region. */

      __afl_area_ptr = __afl_area_initial;
    }
  }

  return 0;
}

void fill_rand_area(void) {
  FILE *rfd;

  rfd = fopen("/dev/urandom", "r");

  if (fread(__gfz_rand_area, RAND_POOL_SIZE, 1, rfd) != 1) {
    FATAL("Unable to get enough rand bytes");
  }
  fclose(rfd);
}

/* This one can be called from user code when deferred forkserver mode
    is enabled. */

void __afl_manual_init(void) {

  static u8 init_done;

  if (!init_done) {

    __gfz_map_area = calloc(__gfz_map_size, 1);
    __gfz_rand_area = malloc(RAND_POOL_SIZE);
    __gfz_rand_idx = 0;
    fill_rand_area();

    __afl_map_shm();
    __afl_start_forkserver();
    init_done = 1;
  }
}

/* Proper initialization routine. */

__attribute__((constructor(CONST_PRIO))) void __afl_auto_init(void) {

  is_persistent = !!getenv(PERSIST_ENV_VAR);

  if (getenv(DEFER_ENV_VAR))
    return;

  __afl_manual_init();
}

/* The following stuff deals with supporting -fsanitize-coverage=trace-pc-guard.
   It remains non-operational in the traditional, plugin-backed LLVM mode.
   For more info about 'trace-pc-guard', see README.llvm.

   The first function (__sanitizer_cov_trace_pc_guard) is called back on every
   edge (as opposed to every basic block). */

void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
  __afl_area_ptr[*guard]++;
}

/* Init callback. Populates instrumentation IDs. Note that we're using
   ID of 0 as a special value to indicate non-instrumented bits. That may
   still touch the bitmap, but in a fairly harmless way. */

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop) {

  u32 inst_ratio = 100;
  u8 *x;

  if (start == stop || *start)
    return;

  x = getenv("AFL_INST_RATIO");
  if (x)
    inst_ratio = atoi(x);

  if (!inst_ratio || inst_ratio > 100) {
    fprintf(stderr, "[-] ERROR: Invalid AFL_INST_RATIO (must be 1-100).\n");
    abort();
  }

  /* Make sure that the first element in the range is always set - we use that
     to avoid duplicate calls (which can happen as an artifact of the underlying
     implementation in LLVM). */

  *(start++) = R(MAP_SIZE - 1) + 1;

  while (start < stop) {

    if (R(100) < inst_ratio)
      *start = R(MAP_SIZE - 1) + 1;
    else
      *start = 0;

    start++;
  }
}
