#!/bin/bash

# Build everything
rm -f /dev/shm/gfzidfile
make -C ../ clean
make -C ../

# Emit LLVM IR of test program with instrumentation
rm -f ./map_key.txt
rm -f /dev/shm/gfzidfile
GFZ_WHITE_LIST=1 ../afl/bin/gfz-clang-fast -I. -S -emit-llvm map_binary.c

# Verify LLVM IR of test program with instrumentation
opt -verify map_binary.ll -o /dev/null

# Build test program with instrumentation
rm -f ./map_key.txt
rm -f /dev/shm/gfzidfile
GFZ_WHITE_LIST=1 ../afl/bin/gfz-clang-fast -I. map_binary.c

# optional "-go" command line option:
#     Feed instrumented test program to afl-fuzz in gFuzz mode

if [ $# -ge 1 ] && [ $1 == "-go" ]
  then
    rm -f ./afl-fuzz.log && touch ./afl-fuzz.log
    GFZ_NUM_ITER=100000 AFL_SKIP_CPUFREQ=1 ../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -G ./a.out 1 2 3
fi

# optional "-t1" command line option:
#     Feed instrumented test program t1 to afl-fuzz in gFuzz mode

if [ $# -ge 1 ] && [ $1 == "-t1" ]
  then
    rm -f /dev/shm/fuzztest
    rm -rf /dev/shm/generated
    rm -f ./afl-fuzz.log && touch ./afl-fuzz.log
    GFZ_NUM_ITER=311296 AFL_SKIP_CPUFREQ=1 ../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -G -g /dev/shm/fuzztest ../tests/t1/gtest
    if [ $# -ge 2 ] && [ $2 == "--test" ]
      then
        ./test.sh -t1
    fi
fi

# optional "-t2" command line option:
#     Feed instrumented test program t2 to afl-fuzz in gFuzz mode

if [ $# -ge 1 ] && [ $1 == "-t2" ]
  then
    rm -f /dev/shm/fuzztest
    rm -rf /dev/shm/generated
    rm -f ./afl-fuzz.log && touch ./afl-fuzz.log
    GFZ_NUM_ITER=1000 AFL_SKIP_CPUFREQ=1 ../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -G -g /dev/shm/fuzztest ../tests/t2/gtest
    if [ $# -ge 2 ] && [ $2 == "--test" ]
      then
        ./test.sh -t2
    fi
fi