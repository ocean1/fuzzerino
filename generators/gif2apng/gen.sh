rm -rf gfz_plot_data
../../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -x png.dict -G -g /dev/shm/fuzztest -p ../../parsers/libpng/contrib/libtests/readpng -- ./gif2apng giftest