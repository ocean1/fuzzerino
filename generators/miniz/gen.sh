rm -rf gfz_plot_data
../../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -G -g /dev/shm/fuzztest -p cmin_targets -c gen_cmdlines