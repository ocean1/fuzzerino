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
#include "../pmparser.h"

#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <malloc.h>
#include <fcntl.h>

/* This is a somewhat ugly hack for the experimental 'trace-pc-guard' mode.
   Basically, we need to make sure that the forkserver is initialized after
   the LLVM-generated runtime initialization pass, not before. */

#ifdef USE_TRACE_PC
#define CONST_PRIO 5
#else
#define CONST_PRIO 0
#endif /* ^USE_TRACE_PC */

/* Globals needed by the injected instrumentation. The __afl_area_initial region
   is used for instrumentation output before __gfz_map_shm() has a chance to
   run. It will end up as .comm, so it shouldn't be too wasteful. */

u8 __afl_area_initial[MAP_SIZE];
u8 *__afl_area_ptr = __afl_area_initial;

__thread u32 __afl_prev_loc;

/* Running in persistent mode? */

static u8 is_persistent;

// override free, generators are short lived programs (http://www.drdobbs.com/cpp/increasing-compiler-speed-by-over-75/240158941)
// https://twitter.com/johnregehr/status/1073801738707070977
// we could also override malloc to provide a little bit more space to reduce SEGFAULTs

void free(void *ptr)
{
}

/* Extern variables which will be linked against by gfz-ld wrapper
   through a command line flag called "--defsym".

   They are filled with data taken from GFZ_IDFILE at the end of the
   compiling process, to avoid LTO. */

extern u32 __gfz_num_locs_defsym;    /* Number of instrumented numeric locations in the target binary. */
extern u32 __gfz_ptr_locs_defsym;    /* Number of instrumented pointer locations in the target binary. */
extern u32 __gfz_branch_locs_defsym; /* Number of instrumented branches in the target binary. */
extern u32 __gfz_total_bbs_defsym;   /* Number of (whitelisted) basic blocks in the target binary. */

/* gFuzz structure to send to the fuzzer. */

struct gfz_data __gfz_data;

/* Numeric locations bitmap SHM */

u16* __gfz_num_map;

/* Pointer locations bitmap SHM */

u16* __gfz_ptr_map;

/* Branch locations bitmap SHM */

u8* __gfz_branch_map;

/* Buffer for ptr dict instrumentation (custom buffer) */

u8* __gfz_ptr_buf;

/* Map for generator coverage */

u8* __gfz_cov_map;

/* Random file descriptor for populating __gfz_rand_area. */

FILE* rfd;

/* "Rand" area gets filled by the thread itself every time we change fuzz
 * strategy or when all values have been used, for example filled with low/high
 * values, or random values */

u8* __gfz_rand_area = NULL;
u32 __gfz_rand_idx;

/* SHM setup. */

static void __afl_map_shm(void) {

  u8 *id_str = getenv(SHM_ENV_VAR);

  // If we're running under AFL, attach to the appropriate region, replacing the
  // early-stage __afl_area_initial region that is needed to allow some really
  // hacky .init code to work correctly in projects such as OpenSSL.

  if (id_str) {

    u32 shm_id = atoi(id_str);

    __afl_area_ptr = shmat(shm_id, NULL, 0);

    // Whooooops.

    if (__afl_area_ptr == (void *)-1)
      _exit(1);

    // Write something into the bitmap so that even with low AFL_INST_RATIO,
    // our parent doesn't give up on us.

    __afl_area_ptr[0] = 1;
  }
}

/* Attach the five SHMs in order to be able to control
   them 'from the outside' (i.e. the fuzzer) */

static void __gfz_map_shm(void) {

  /* Attach __gfz_num_map */

  u8 *id_str = getenv(GFZ_NUM_SHM_ENV_VAR);

  if (id_str) {
    u32 shm_id = atoi(id_str);
    __gfz_num_map = shmat(shm_id, NULL, 0);

    // Whooooops.
    if (__gfz_num_map == (void *)-1) {
      _exit(1);
    }
  } else {
    __gfz_num_map = calloc(GFZ_NUM_MAP_SIZE, 1);
  }

  /* Attach __gfz_ptr_map */

  id_str = getenv(GFZ_PTR_SHM_ENV_VAR);

  if (id_str) {
    u32 shm_id = atoi(id_str);
    __gfz_ptr_map = shmat(shm_id, NULL, 0);

    // Whooooops.
    if (__gfz_ptr_map == (void *)-1) {
      _exit(1);
    }
  } else {
    __gfz_ptr_map = calloc(GFZ_PTR_MAP_SIZE, 1);
  }

  /* Attach __gfz_branch_map */

  id_str = getenv(GFZ_BNC_SHM_ENV_VAR);

  if (id_str) {
    u32 shm_id = atoi(id_str);
    __gfz_branch_map = shmat(shm_id, NULL, 0);

    // Whooooops.
    if (__gfz_branch_map == (void *)-1) {
      _exit(1);
    }
  } else {
    __gfz_branch_map = calloc(GFZ_BNC_MAP_SIZE, 1);
  }

  /* Attach __gfz_ptr_buf */

  id_str = getenv(GFZ_BUF_SHM_ENV_VAR);

  if (id_str) {
    u32 shm_id = atoi(id_str);
    __gfz_ptr_buf = shmat(shm_id, NULL, 0);

    // Whooooops.
    if (__gfz_ptr_buf == (void *)-1) {
      _exit(1);
    }
  } else {
    __gfz_ptr_buf = malloc(GFZ_PTR_BUF_SIZE);
    
    int i = 0;
    for (i = 0; i < GFZ_PTR_BUF_SIZE; ++i) {
      __gfz_ptr_buf[i] = 'A';
    }
  }

  /* Attach __gfz_cov_map */

  id_str = getenv(GFZ_COV_SHM_ENV_VAR);

  if (id_str) {
    u32 shm_id = atoi(id_str);
    __gfz_cov_map = shmat(shm_id, NULL, 0);

    // Whooooops.
    if (__gfz_cov_map == (void *)-1) {
      _exit(1);
    }

  } else {
    __gfz_cov_map = calloc(GFZ_COV_MAP_SIZE, 1);
  }

}

/*
  Parses /proc/self/maps and calls mprotect on all the
  memory regions in order to make everything writable.
  (actually rwx)

  This is to have less crashes when instrumenting pointers
  and buffers.

  Generators are short lived programs.
  See above where free is overwritten.

  Process memory map parsing library from:

  https://github.com/ouadev/proc_maps_parser
*/

void __gfz_make_writable() {

  procmaps_iterator* maps = pmparser_parse(-1);

  if (maps == NULL){
    WARNF("[map]: cannot parse the memory map");
    return;
  }

  procmaps_struct* maps_tmp = NULL;
  
  while( (maps_tmp = pmparser_next(maps)) != NULL){
    mprotect(maps_tmp->addr_start,
             maps_tmp->addr_end - maps_tmp->addr_start,
             PROT_READ|PROT_WRITE|PROT_EXEC);
  }

  pmparser_free(maps);

}

/* Fork server logic. */

static void __gfz_start_forkserver(void) {

  static u8 tmp[4];
  s32 child_pid;
  u8 child_stopped = 0;

  /* Phone home and tell the parent that we're OK. If parent isn't there,
     assume we're not running in forkserver mode and just execute program. */

  if (write(FORKSRV_FD + 1, tmp, 4) != 4) {
    
    /*

      Code used for instrumentation testing.

      It fills the maps area with custom map files, if present.
      The custom maps are searched in files in the current folder called

          "gfz_num" for the numeric locations map
          "gfz_ptr" for the pointer locations map
          "gfz_bnc" for the branch locations map

    */

    FILE *gfz_num_file = fopen("./gfz_num", "rb");

    if (gfz_num_file) {
    
      ssize_t n_bytes = fread(__gfz_num_map, GFZ_NUM_MAP_SIZE, 1, gfz_num_file);
    
      if (n_bytes <= 0)
        printf("[-] Error reading gfz_num. (%ld)\n", n_bytes);
      else
        printf("[+] gfz_num read.");

      /* When a custom map is present, make the whole process
         memory writable, just as if we were fuzzing. */

      __gfz_make_writable();
    
    } else {
    
      /* If no custom map is present, just fill it with ones
         to execute normal program behavior. */

      int i = 0;

      for (i = 0; i < (GFZ_NUM_MAP_SIZE / 2); ++i) {
        __gfz_num_map[i] = 1;
      }
      
      /* Don't change rwx of the process memory in this case
         so that the instrumentation is fairly transparent
         and the program can still be used normally when
         NOT fuzzing, or testing with a custom map. */

    }

    FILE *gfz_ptr_file = fopen("./gfz_ptr", "rb");

    if (gfz_ptr_file) {
    
      ssize_t n_bytes = fread(__gfz_ptr_map, GFZ_PTR_MAP_SIZE, 1, gfz_ptr_file);
    
      if (n_bytes <= 0)
        printf("[-] Error reading gfz_ptr. (%ld)\n", n_bytes);
      else
        printf("[+] gfz_ptr read.");

      /* When a custom map is present, make the whole process
         memory writable, just as if we were fuzzing. */

      if (!gfz_num_file)
        __gfz_make_writable();
    
    } else {
    
      /* If no custom map is present, just fill it with ones
         to execute normal program behavior. */

      int i = 0;

      for (i = 0; i < (GFZ_PTR_MAP_SIZE / 2); ++i) {
        __gfz_ptr_map[i] = 1;
      }
      
      /* Don't change rwx of the process memory in this case
         so that the instrumentation is fairly transparent
         and the program can still be used normally when
         NOT fuzzing, or testing with a custom map. */

    }

    FILE *gfz_bnc_file = fopen("gfz_bnc", "rb");

    if (gfz_bnc_file) {

      ssize_t n_bytes = fread(__gfz_branch_map, GFZ_BNC_MAP_SIZE, 1, gfz_bnc_file);
    
      if (n_bytes <= 0)
        printf("[-] Error reading gfz_bnc. (%ld)\n", n_bytes);
      else
        printf("[+] gfz_bnc read.");

      /* When a custom map is present, make the whole process
         memory writable, just as if we were fuzzing. */

      if (!gfz_num_file && !gfz_ptr_file)
        __gfz_make_writable();

    } else {

      int i = 0;

      for (i = 0; i < GFZ_BNC_MAP_SIZE; ++i) {
        __gfz_branch_map[i] = 1;
      }

      /* Don't change rwx of the process memory in this case
         so that the instrumentation is fairly transparent
         and the program can still be used normally when
         NOT fuzzing, or testing with a custom map. */
    
    }

    /* End code for instrumentation testing */
    return;
  }

  __gfz_make_writable();

  /* Send __gfz_data to the fuzzer. */

  __gfz_data.gfz_num_locs = (u32) &__gfz_num_locs_defsym;
  __gfz_data.gfz_ptr_locs = (u32) &__gfz_ptr_locs_defsym;
  __gfz_data.gfz_branch_locs = (u32) &__gfz_branch_locs_defsym;
  __gfz_data.gfz_total_bbs = (u32) &__gfz_total_bbs_defsym;

  if (write(FORKSRV_FD + 1, &__gfz_data, sizeof(struct gfz_data)) != sizeof(struct gfz_data))
    FATAL("Unable to write __gfz_data to the fuzzer! (did you activate gFuzz mode?)");

  /* Forkserver loop. */

  int i = 0;

  while(1) {

    // u8 *inststat = getenv("GFZ_STAT_VAL");
    // u8 insval = (inststat == NULL) ? 1 : atoi(inststat);
    
    u32 was_killed;
    int status;

    /* Wait for parent by reading from the pipe. Abort if read fails. */

    if (read(FORKSRV_FD, &was_killed, 4) != 4) _exit(1);

    /* If we stopped the child in persistent mode, but there was a race
       condition and afl-fuzz already issued SIGKILL, write off the old
       process. */

    if ( child_stopped ) { //&& was_killed) {
      child_stopped = 0;
      if (waitpid(child_pid, &status, 0) < 0)
        _exit(1);
    }

    if ( !child_stopped ) {

      /* Once woken up, create a clone of our process. */

      child_pid = fork();
      if (child_pid < 0)
        _exit(1);

      /* In child process: close fds, resume execution. */

      if (!child_pid) {
        close(FORKSRV_FD);
        close(FORKSRV_FD + 1);
        return;
      }

      ++i;

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

  } // end while(1)

  fclose(rfd);

}

/* A simplified persistent mode handler, used as explained in README.llvm. */

int __gfz_persistent_loop(unsigned int max_cnt) {

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

/* 
   This one can be called from user code when deferred forkserver mode
   is enabled.
*/

void __gfz_manual_init(void) {

  static u8 init_done;

  if (!init_done) {

    __gfz_map_shm();

    rfd = fopen("/dev/urandom", "r");

    __gfz_rand_area = malloc(GFZ_RAND_POOL_SIZE);
    __gfz_rand_idx = 0;
    
    if (fread(__gfz_rand_area, GFZ_RAND_POOL_SIZE, 1, rfd) != 1) {
      FATAL("Unable to get enough rand bytes");
    }

    __afl_map_shm();
    __gfz_start_forkserver();
    init_done = 1;
  }

}

/* Proper initialization routine. */

__attribute__((constructor(CONST_PRIO))) void __gfz_auto_init(void) {

  is_persistent = !!getenv(PERSIST_ENV_VAR);

  if (getenv(DEFER_ENV_VAR))
    return;

  __gfz_manual_init();

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
