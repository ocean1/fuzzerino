/*
   american fuzzy lop - LLVM-mode wrapper for ld
   ------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres.

   Copyright 2015, 2016 Google Inc. All rights reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at:

     http://www.apache.org/licenses/LICENSE-2.0

   This program is a drop-in replacement for ld.
   It reads the number of instrumented locations from GFZ_IDFILE,
   adds a flag that defines a symbol containing that value in the
   linked binary, and calls the real linker.

 */

#define AFL_MAIN

#include "../config.h"
#include "../types.h"
#include "../debug.h"
#include "../alloc-inl.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

static u8** cc_params;              /* Parameters passed to the real CC  */
static u32  cc_par_cnt = 1;         /* Param count, including argv0      */

/* Main entry point */

int main(int argc, char** argv) {

  if (isatty(2) && !getenv("AFL_QUIET")) {

#ifdef USE_TRACE_PC
    SAYF(cCYA "gfz-ld [tpcg] " cBRI VERSION  cRST " by <limi7break>\n");
#else
    SAYF(cCYA "gfz-ld " cBRI VERSION  cRST " by <limi7break>\n");
#endif /* ^USE_TRACE_PC */

  }

  if (argc < 2) {

    SAYF("\n"
         "This is a helper application for gfz-fuzz. It serves as a drop-in replacement\n"
         "for ld, letting you recompile third-party code with the required runtime\n"
         "instrumentation."

         "The purpose of this wrapper is to read the number of instrumented locations"
         "and basic blocks from %s, add a flag that defines a symbol containing"
         "that value in the linked binary, and calls the real linker.", GFZ_IDFILE);

    exit(1);

  }

  u32 __gfz_map_locs;
  u32 __gfz_branch_locs;
  u32 __gfz_total_bbs;

  cc_params = ck_alloc((argc + 128) * sizeof(u8*));
  cc_params[0] = "ld";

  while (--argc) {
    u8* cur = *(++argv);
    cc_params[cc_par_cnt++] = cur;
  }

  /* Read the number of instrumented locations from GFZ_IDFILE */

  int idfd = open(GFZ_IDFILE, O_CREAT | O_RDWR,
                  S_IRUSR | S_IWUSR | S_IRGRP);

  if (read(idfd, &__gfz_map_locs, sizeof(__gfz_map_locs)) != sizeof(__gfz_map_locs))
    FATAL("[-] Cannot read number of locations!");

  if (read(idfd, &__gfz_branch_locs, sizeof(__gfz_branch_locs)) != sizeof(__gfz_branch_locs))
    FATAL("[-] Cannot read number of basic blocks!");

  if (read(idfd, &__gfz_total_bbs, sizeof(__gfz_total_bbs)) != sizeof(__gfz_total_bbs))
    FATAL("[-] Cannot read number of basic blocks!");

  close(idfd);

  /* Define symbol */

  cc_params[cc_par_cnt++] = "--defsym";
  cc_params[cc_par_cnt++] = alloc_printf("__gfz_map_locs_defsym=%u", __gfz_map_locs);

  OKF("Read %u instrumented locations from %s.", __gfz_map_locs, GFZ_IDFILE);

  /* Define symbol */

  cc_params[cc_par_cnt++] = "--defsym";
  cc_params[cc_par_cnt++] = alloc_printf("__gfz_branch_locs_defsym=%u", __gfz_branch_locs);

  OKF("Read %u instrumented branches from %s.", __gfz_branch_locs, GFZ_IDFILE);

  /* Define symbol */

  cc_params[cc_par_cnt++] = "--defsym";
  cc_params[cc_par_cnt++] = alloc_printf("__gfz_total_bbs_defsym=%u", __gfz_total_bbs);

  OKF("Read %u basic blocks from %s.", __gfz_total_bbs, GFZ_IDFILE);

  /* Execute real linker */

  execvp(cc_params[0], (char**)cc_params);

  FATAL("Oops, failed to execute '%s' - check your PATH", cc_params[0]);

  return 0;

}
