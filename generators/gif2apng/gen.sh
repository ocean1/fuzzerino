../../afl/bin/afl-fuzz -b gfz_ban_map -i in -o out -m10000 -t5000 -G -g /dev/shm/fuzztest -- ./gif2apng giftest
