/*
   american fuzzy lop - vaguely configurable bits
   ----------------------------------------------

   Written and maintained by Michal Zalewski <lcamtuf@google.com>

   Copyright 2013, 2014, 2015, 2016 Google Inc. All rights reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

 */

#ifndef _HAVE_CONFIG_H
#define _HAVE_CONFIG_H

#include "types.h"

/* Version string: */

#define VERSION             "1.00"

/******************************************************
 *                                                    *
 *  Settings that may be of interest to power users:  *
 *                                                    *
 ******************************************************/

/* Comment out to disable terminal colors (note that this makes afl-analyze
   a lot less nice): */

#define USE_COLOR

/* Comment out to disable fancy ANSI boxes and use poor man's 7-bit UI: */

#define FANCY_BOXES

/* Default timeout for fuzzed code (milliseconds). This is the upper bound,
   also used for detecting hangs; the actual value is auto-scaled: */

#define EXEC_TIMEOUT        2000

/* Timeout rounding factor when auto-scaling (milliseconds): */

#define EXEC_TM_ROUND       20

/* Default memory limit for child process (MB): */

#ifndef __x86_64__
#  define MEM_LIMIT         25
#else
#  define MEM_LIMIT         50
#endif /* ^!__x86_64__ */

/* Default memory limit when running in QEMU mode (MB): */

#define MEM_LIMIT_QEMU      200

/* Number of calibration cycles per every new test case (and for test
   cases that show variable behavior): */

#define CAL_CYCLES          8
#define CAL_CYCLES_LONG     40

/* Number of subsequent timeouts before abandoning an input file: */

#define TMOUT_LIMIT         250

/* Maximum number of unique hangs or crashes to record: */

#define KEEP_UNIQUE_HANG    500
#define KEEP_UNIQUE_CRASH   5000

/* Baseline number of random tweaks during a single 'havoc' stage: */

#define HAVOC_CYCLES        256
#define HAVOC_CYCLES_INIT   1024

/* Maximum multiplier for the above (should be a power of two, beware
   of 32-bit int overflows): */

#define HAVOC_MAX_MULT      16

/* Absolute minimum number of havoc cycles (after all adjustments): */

#define HAVOC_MIN           16

/* Maximum stacking for havoc-stage tweaks. The actual value is calculated
   like this: 

   n = random between 1 and HAVOC_STACK_POW2
   stacking = 2^n

   In other words, the default (n = 7) produces 2, 4, 8, 16, 32, 64, or
   128 stacked tweaks: */

#define HAVOC_STACK_POW2    7

/* Caps on block sizes for cloning and deletion operations. Each of these
   ranges has a 33% probability of getting picked, except for the first
   two cycles where smaller blocks are favored: */

#define HAVOC_BLK_SMALL     32
#define HAVOC_BLK_MEDIUM    128
#define HAVOC_BLK_LARGE     1500

/* Extra-large blocks, selected very rarely (<5% of the time): */

#define HAVOC_BLK_XL        32768

/* Probabilities of skipping non-favored entries in the queue, expressed as
   percentages: */

#define SKIP_TO_NEW_PROB    99 /* ...when there are new, pending favorites */
#define SKIP_NFAV_OLD_PROB  95 /* ...no new favs, cur entry already fuzzed */
#define SKIP_NFAV_NEW_PROB  75 /* ...no new favs, cur entry not fuzzed yet */

/* Splicing cycle count: */

#define SPLICE_CYCLES       15

/* Nominal per-splice havoc cycle length: */

#define SPLICE_HAVOC        32

/* Maximum offset for integer addition / subtraction stages: */

#define ARITH_MAX           35

/* Limits for the test case trimmer. The absolute minimum chunk size; and
   the starting and ending divisors for chopping up the input file: */

#define TRIM_MIN_BYTES      4
#define TRIM_START_STEPS    16
#define TRIM_END_STEPS      1024

/* Maximum size of input file, in bytes (keep under 100MB): */

#define MAX_FILE            (1 * 1024 * 1024)

/* The same, for the test case minimizer: */

#define TMIN_MAX_FILE       (10 * 1024 * 1024)

/* Block normalization steps for afl-tmin: */

#define TMIN_SET_MIN_SIZE   4
#define TMIN_SET_STEPS      128

/* Maximum dictionary token size (-x), in bytes: */

#define MAX_DICT_FILE       128

/* Length limits for auto-detected dictionary tokens: */

#define MIN_AUTO_EXTRA      3
#define MAX_AUTO_EXTRA      32

/* Maximum number of user-specified dictionary tokens to use in deterministic
   steps; past this point, the "extras/user" step will be still carried out,
   but with proportionally lower odds: */

#define MAX_DET_EXTRAS      200

/* Maximum number of auto-extracted dictionary tokens to actually use in fuzzing
   (first value), and to keep in memory as candidates. The latter should be much
   higher than the former. */

#define USE_AUTO_EXTRAS     50
#define MAX_AUTO_EXTRAS     (USE_AUTO_EXTRAS * 10)

/* Scaling factor for the effector map used to skip some of the more
   expensive deterministic steps. The actual divisor is set to
   2^EFF_MAP_SCALE2 bytes: */

#define EFF_MAP_SCALE2      3

/* Minimum input file length at which the effector logic kicks in: */

#define EFF_MIN_LEN         128

/* Maximum effector density past which everything is just fuzzed
   unconditionally (%): */

#define EFF_MAX_PERC        90

/* UI refresh frequency (Hz): */

#define UI_TARGET_HZ        5

/* Fuzzer stats file and plot update intervals (sec): */

#define STATS_UPDATE_SEC    60
#define PLOT_UPDATE_SEC     5

/* Smoothing divisor for CPU load and exec speed stats (1 - no smoothing). */

#define AVG_SMOOTHING       16

/* Sync interval (every n havoc cycles): */

#define SYNC_INTERVAL       5

/* Output directory reuse grace period (minutes): */

#define OUTPUT_GRACE        25

/* Uncomment to use simple file names (id_NNNNNN): */

// #define SIMPLE_FILES

/* List of interesting values to use in fuzzing. */

#define INTERESTING_8 \
  -128,          /* Overflow signed 8-bit when decremented  */ \
  -1,            /*                                         */ \
   0,            /*                                         */ \
   1,            /*                                         */ \
   16,           /* One-off with common buffer size         */ \
   32,           /* One-off with common buffer size         */ \
   64,           /* One-off with common buffer size         */ \
   100,          /* One-off with common buffer size         */ \
   127           /* Overflow signed 8-bit when incremented  */

#define INTERESTING_16 \
  -32768,        /* Overflow signed 16-bit when decremented */ \
  -129,          /* Overflow signed 8-bit                   */ \
   128,          /* Overflow signed 8-bit                   */ \
   255,          /* Overflow unsig 8-bit when incremented   */ \
   256,          /* Overflow unsig 8-bit                    */ \
   512,          /* One-off with common buffer size         */ \
   1000,         /* One-off with common buffer size         */ \
   1024,         /* One-off with common buffer size         */ \
   4096,         /* One-off with common buffer size         */ \
   32767         /* Overflow signed 16-bit when incremented */

#define INTERESTING_32 \
  -2147483648LL, /* Overflow signed 32-bit when decremented */ \
  -100663046,    /* Large negative number (endian-agnostic) */ \
  -32769,        /* Overflow signed 16-bit                  */ \
   32768,        /* Overflow signed 16-bit                  */ \
   65535,        /* Overflow unsig 16-bit when incremented  */ \
   65536,        /* Overflow unsig 16 bit                   */ \
   100663045,    /* Large positive number (endian-agnostic) */ \
   2147483647    /* Overflow signed 32-bit when incremented */

/***********************************************************
 *                                                         *
 *  Really exotic stuff you probably don't want to touch:  *
 *                                                         *
 ***********************************************************/

/* Call count interval between reseeding the libc PRNG from /dev/urandom: */

#define RESEED_RNG          10000

/* Maximum line length passed from GCC to 'as' and used for parsing
   configuration files: */

#define MAX_LINE            8192

/* Environment variable used to pass SHM ID to the called program. */

#define SHM_ENV_VAR         "__AFL_SHM_ID"

/* Other less interesting, internal-only variables. */

#define CLANG_ENV_VAR       "__AFL_CLANG_MODE"
#define AS_LOOP_ENV_VAR     "__AFL_AS_LOOPCHECK"
#define PERSIST_ENV_VAR     "__AFL_PERSISTENT"
#define DEFER_ENV_VAR       "__AFL_DEFER_FORKSRV"

/* In-code signatures for deferred and persistent mode. */

#define PERSIST_SIG         "##SIG_AFL_PERSISTENT##"
#define DEFER_SIG           "##SIG_AFL_DEFER_FORKSRV##"

/* Distinctive bitmap signature used to indicate failed execution: */

#define EXEC_FAIL_SIG       0xfee1dead

/* Distinctive exit code used to indicate MSAN trip condition: */

#define MSAN_ERROR          86

/* Designated file descriptors for forkserver commands (the application will
   use FORKSRV_FD and FORKSRV_FD + 1): */

#define FORKSRV_FD          198

/* Fork server init timeout multiplier: we'll wait the user-selected
   timeout plus this much for the fork server to spin up. */

#define FORK_WAIT_MULT      10

/* Calibration timeout adjustments, to be a bit more generous when resuming
   fuzzing sessions or trying to calibrate already-added internal finds.
   The first value is a percentage, the other is in milliseconds: */

#define CAL_TMOUT_PERC      125
#define CAL_TMOUT_ADD       50

/* Number of chances to calibrate a case before giving up: */

#define CAL_CHANCES         3

/* Map size for the traced binary (2^MAP_SIZE_POW2). Must be greater than
   2; you probably want to keep it under 18 or so for performance reasons
   (adjusting AFL_INST_RATIO when compiling is probably a better way to solve
   problems with complex programs). You need to recompile the target binary
   after changing this - otherwise, SEGVs may ensue. */

#define MAP_SIZE_POW2       16
#define MAP_SIZE            (1 << MAP_SIZE_POW2)

/* Maximum allocator request size (keep well under INT_MAX): */

#define MAX_ALLOC           0x40000000

/* A made-up hashing seed: */

#define HASH_CONST          0xa5b35705

/* Constants for afl-gotcpu to control busy loop timing: */

#define  CTEST_TARGET_MS    5000
#define  CTEST_CORE_TRG_MS  1000
#define  CTEST_BUSY_CYCLES  (10 * 1000 * 1000)

/* Uncomment this to use inferior block-coverage-based instrumentation. Note
   that you need to recompile the target binary for this to have any effect: */

// #define COVERAGE_ONLY

/* Uncomment this to ignore hit counts and output just one bit per tuple.
   As with the previous setting, you will need to recompile the target
   binary: */

// #define SKIP_COUNTS

/* Uncomment this to use instrumentation data to record newly discovered paths,
   but do not use them as seeds for fuzzing. This is useful for conveniently
   measuring coverage that could be attained by a "dumb" fuzzing algorithm: */

// #define IGNORE_FINDS



/* gFuzz stuff! */

/* Environment variable used to pass SHM ID to the called program. */

#define GFZ_NUM_SHM_ENV_VAR     "__GFZ_NUM_SHM_ID"
#define GFZ_PTR_SHM_ENV_VAR     "__GFZ_PTR_SHM_ID"
#define GFZ_BNC_SHM_ENV_VAR     "__GFZ_BNC_SHM_ID"
#define GFZ_BUF_SHM_ENV_VAR     "__GFZ_BUF_SHM_ID"
#define GFZ_COV_SHM_ENV_VAR     "__GFZ_COV_SHM_ID"

/* Size for map containing numeric locations (2MB) */

#define GFZ_NUM_MAP_SIZE_POW2   21
#define GFZ_NUM_MAP_SIZE        (1 << GFZ_NUM_MAP_SIZE_POW2)

/* Size for map containing pointer locations (2MB) */

#define GFZ_PTR_MAP_SIZE_POW2   21
#define GFZ_PTR_MAP_SIZE        (1 << GFZ_PTR_MAP_SIZE_POW2)

/* Size for map containing branch locations (16k) */

#define GFZ_BNC_MAP_SIZE_POW2   14
#define GFZ_BNC_MAP_SIZE        (1 << GFZ_BNC_MAP_SIZE_POW2)

/* Size for helper buffer for ptr instrumentation (128 bytes) */

#define GFZ_PTR_BUF_SIZE_POW2   7
#define GFZ_PTR_BUF_SIZE        (1 << GFZ_PTR_BUF_SIZE_POW2)

/* Size for generator coverage map (64k) */

#define GFZ_COV_MAP_SIZE_POW2   16
#define GFZ_COV_MAP_SIZE        (1 << GFZ_COV_MAP_SIZE_POW2)

/* Size for pool containing values to be used in mutations, 4KB seems good */

#define GFZ_RAND_POOL_SIZE_POW2 12
#define GFZ_RAND_POOL_SIZE      (1 << GFZ_RAND_POOL_SIZE_POW2)

/* File for keeping track of instrumented locations */

#define GFZ_IDFILE              "/dev/shm/gfzidfile"

/* Output file size limit (134MB) */

#define GFZ_OUTPUT_LIMIT_POW2   27
#define GFZ_OUTPUT_LIMIT        (1 << GFZ_OUTPUT_LIMIT_POW2)

/* Random stuff */

#define GFZ_FTE_BAN_RATIO       0.5         /* Fault to exec ratio for banning a location during dry run(s)              */
#define GFZ_UPDATE_SEC          5           /* Time interval between coverage and plot file update                       */
#define GFZ_HAVOC_TMOUT_SEC     40          /* Time after which, if consistently slow, maps are reset (havoc)            */
#define GFZ_HAVOC_BRANCH_EXECS  1000        /* Number of executions between every branch location flip (havoc)           */
#define GFZ_HAVOC_CMDLINE_EXECS 100000      /* Number of executions between every cmdline change (havoc)                 */
#define GFZ_MAX_DICT_ENTRIES    100         /* Maximum dictionary entries                                                */
#define GFZ_MAX_MIN_TARGETS     10          /* Maximum minimization targets                                              */
#define GFZ_MAX_GEN_CMDLINES    10          /* Maximum generator cmdlines                                                */

/* Mutations */

#define GFZ_N_MUTATIONS         13

#define GFZ_KEEP_ORIGINAL       1           // 0000 0000 0000 0001

/* Numeric mutations */

#define GFZ_PLUS_ONE            2           // 0000 0000 0000 0010
#define GFZ_MINUS_ONE           4           // 0000 0000 0000 0100
#define GFZ_INTERESTING_1       8           // 0000 0000 0000 1000
#define GFZ_INTERESTING_2       16          // 0000 0000 0001 0000
#define GFZ_INTERESTING_3       32          // 0000 0000 0010 0000
#define GFZ_INTERESTING_4       64          // 0000 0000 0100 0000
#define GFZ_INTERESTING_5       128         // 0000 0000 1000 0000
#define GFZ_INTERESTING_6       256         // 0000 0001 0000 0000
#define GFZ_INTERESTING_7       512         // 0000 0010 0000 0000
#define GFZ_INTERESTING_8       1024        // 0000 0100 0000 0000
#define GFZ_PLUS_MAX            2048        // 0000 1000 0000 0000
#define GFZ_PLUS_RAND           4096        // 0001 0000 0000 0000
#define GFZ_RESERVED_1          8192        // 0010 0000 0000 0000
#define GFZ_RESERVED_2          16384       // 0100 0000 0000 0000
#define GFZ_RESERVED_3          32768       // 1000 0000 0000 0000

/* Pointer mutations */

#define GFZ_BITFLIP             2           // 0000 0000 0000 0010
#define GFZ_BYTEFLIP            4           // 0000 0000 0000 0100
#define GFZ_ARITH               8           // 0000 0000 0000 1000
#define GFZ_INTERESTING         16          // 0000 0000 0001 0000
#define GFZ_CUSTOM_BUF          32          // 0000 0000 0010 0000
#define GFZ_LEN_1               64          // 0000 0000 0100 0000
#define GFZ_LEN_2               128         // 0000 0000 1000 0000
#define GFZ_LEN_3               256         // 0000 0001 0000 0000
#define GFZ_STRIDE_LEN_1        512         // 0000 0010 0000 0000
#define GFZ_STRIDE_LEN_2        1024        // 0000 0100 0000 0000
#define GFZ_STRIDE_LEN_3        2048        // 0000 1000 0000 0000
#define GFZ_STRIDE_LEN_4        4096        // 0001 0000 0000 0000
#define GFZ_STRIDE_LEN_5        8192        // 0010 0000 0000 0000
#define GFZ_STRIDE_LEN_6        16384       // 0100 0000 0000 0000
#define GFZ_STRIDE_LEN_7        32768       // 1000 0000 0000 0000

/* Deterministic mutation combinations to use in dry run */

#define GFZ_NUM_DRY_NUMERIC     24

#define GFZ_DRY_NUMERIC \
  GFZ_PLUS_ONE | GFZ_KEEP_ORIGINAL,                         \
  GFZ_MINUS_ONE | GFZ_KEEP_ORIGINAL,                        \
  GFZ_INTERESTING_1 | GFZ_KEEP_ORIGINAL,                    \
  GFZ_INTERESTING_2 | GFZ_KEEP_ORIGINAL,                    \
  GFZ_INTERESTING_3 | GFZ_KEEP_ORIGINAL,                    \
  GFZ_INTERESTING_4 | GFZ_KEEP_ORIGINAL,                    \
  GFZ_INTERESTING_5 | GFZ_KEEP_ORIGINAL,                    \
  GFZ_INTERESTING_6 | GFZ_KEEP_ORIGINAL,                    \
  GFZ_INTERESTING_7 | GFZ_KEEP_ORIGINAL,                    \
  GFZ_INTERESTING_8 | GFZ_KEEP_ORIGINAL,                    \
  GFZ_PLUS_MAX | GFZ_KEEP_ORIGINAL,                         \
  GFZ_PLUS_RAND | GFZ_KEEP_ORIGINAL,                        \
  GFZ_PLUS_ONE,                                             \
  GFZ_MINUS_ONE,                                            \
  GFZ_INTERESTING_1,                                        \
  GFZ_INTERESTING_2,                                        \
  GFZ_INTERESTING_3,                                        \
  GFZ_INTERESTING_4,                                        \
  GFZ_INTERESTING_5,                                        \
  GFZ_INTERESTING_6,                                        \
  GFZ_INTERESTING_7,                                        \
  GFZ_INTERESTING_8,                                        \
  GFZ_PLUS_MAX,                                             \
  GFZ_PLUS_RAND

#define GFZ_NUM_DRY_POINTERS    12

#define GFZ_DRY_POINTERS \
  GFZ_BITFLIP,                                              \
  GFZ_BITFLIP | GFZ_LEN_1,                                  \
  GFZ_BITFLIP | GFZ_LEN_2,                                  \
  GFZ_BITFLIP | GFZ_LEN_1 | GFZ_LEN_2,                      \
  GFZ_BITFLIP | GFZ_LEN_3,                                  \
  GFZ_BITFLIP | GFZ_LEN_1 | GFZ_LEN_3,                      \
  GFZ_BITFLIP | GFZ_LEN_2 | GFZ_LEN_3,                      \
  GFZ_BITFLIP | GFZ_LEN_1 | GFZ_LEN_2 | GFZ_LEN_3,          \
  GFZ_BYTEFLIP,                                             \
  GFZ_BYTEFLIP | GFZ_LEN_1,                                 \
  GFZ_BYTEFLIP | GFZ_LEN_2,                                 \
  GFZ_BYTEFLIP | GFZ_LEN_1 | GFZ_LEN_2

#endif /* ! _HAVE_CONFIG_H */