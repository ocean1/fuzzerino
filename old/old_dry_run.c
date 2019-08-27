/* 

  === Dry run. ===

*/

if ( ban_read_error || !ban_file ) {

  u8 tmp[21];
  stage_name = tmp;

  u64 slow_since = 0;
  u8 timeout_ban = 0;

  u32 loc = 0;
  u16 mut = 0;

  for ( loc = 0; loc < __gfz_map_locs; ++loc ) {

    u16 total_faults = 0;
    u16 loc_iter = (1 << GFZ_N_MUTATIONS) - 1;

    sprintf(tmp, "dry %u/%u", loc, __gfz_map_locs);

    for ( mut = 1; mut < loc_iter; ++mut ) {

      show_stats();

      __gfz_map_ptr[loc]++;

      fault = run_target(use_argv, exec_tmout);

      if (unlikely(!(total_execs % GFZ_COV_REFRESH_RATE))) {
        __gfz_covered = 0;
        for (i = 0; i < __gfz_total_bbs; ++i)
          __gfz_covered += __gfz_cov_map[i];
      }

      if ( fault == FAULT_NONE ) {
        /* Do not save files generated during dry run.
           The only purpose of the dry run is to have a better
           havoc phase afterwards. */

        //if ( gen_file && !stat(gen_file, &st) && st.st_size != 0 ) {
          //// Save output in gen_dir
          //u8 *fn = alloc_printf("%s/dry_%d_%d", gen_dir, loc, __gfz_map_ptr[loc]);
          //rename(gen_file, fn);
          //ck_free(fn);
        //}
      } else {
        ++total_faults;
      }

      switch (fault) {
        case FAULT_TMOUT:
          ++total_tmouts;
          break;
        case FAULT_CRASH:
          ++total_crashes;
          break;
      }

      //if (gen_file)
      //  remove(gen_file);

      /* If applying mutations to this location slows the fuzzer for
         more than 30 seconds continuously, ban it straight away.

         (probably it can be done better)*/

      if ( avg_exec < 100 ) {
        if (!slow_since) {
          slow_since = get_cur_time();
        } else if ( get_cur_time() > ((GFZ_BAN_TMOUT_SEC * 1000) + slow_since) ) {
          timeout_ban = 1;
          break;
        }
      } else {
        slow_since = 0;
      }

    } // end mutation loop

    /* Ban location because of too many faults
       or because of timeout. */

    if ( timeout_ban ||
         ((total_faults / (float)loc_iter) > GFZ_BAN_RATIO) ) {

      __gfz_ban_ptr[loc] = 1;
      __gfz_ban_locs++;

    }

    // fprintf(log_file, "\n\n=== loc %u\n    faults: %u\n    ratio: %.2f\n    banned: %u\n    timeout: %u", loc, total_faults, (total_faults / (float)loc_iter), __gfz_ban_ptr[loc], timeout_ban);

    /* Restore location to GFZ_KEEP_ORIGINAL
       before continuing with the next location */

    __gfz_map_ptr[loc] = GFZ_KEEP_ORIGINAL;

    slow_since  = 0;
    timeout_ban = 0;

  } // end location loop

  /* Dump ban map */

  FILE *map_file = fopen("./gfz_ban_map", "wb");
  fwrite(__gfz_ban_ptr, __gfz_map_locs * sizeof(u8), 1, map_file);

} // if ( ban_read_error || !ban_file )