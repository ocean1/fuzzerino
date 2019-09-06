rm -rf gfz_plot_data
../../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -g /dev/shm/miniz -p cmin_targets $@ -- ./miniz 16 16