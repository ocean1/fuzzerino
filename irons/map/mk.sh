rm -f /dev/shm/gfzidfile
make -C ../../ clean
make -C ../../
rm -f /dev/shm/gfzidfile
rm -f ./map_key.txt
GFZ_WHITE_LIST=1 ../../afl/bin/gfz-clang-fast -I. map_binary.c
rm -f /dev/shm/gfzidfile
rm -f ./map_key.txt
GFZ_WHITE_LIST=1 ../../afl/bin/gfz-clang-fast -I. -S -emit-llvm map_binary.c
opt -verify map_binary.ll

if [ $# -eq 1 ] && [ $1 == "-go" ]
  then
    GFZ_NUM_ITER=100000 AFL_SKIP_CPUFREQ=1 ../../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -G ./a.out
fi
