../../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -G -g /dev/shm/fuzztest -c ../../parsers/libpng/contrib/libtests/readpng -b gfz_ban_map -- ./examples/ex6_gfz 16 16