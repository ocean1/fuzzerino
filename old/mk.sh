#!/bin/bash

# Build everything
rm -f /dev/shm/gfzidfile
make -C ../ clean
make -C ../

float="false"
run=""
test="false"

CFILE="map_binary.c"
LLFILE="map_binary.ll"

while getopts ":fr12t" opt
do
  case $opt in
  f) float="true";;
  r) run="r";;
  1) run="1";;
  2) run="2";;
  t) test="true";;
  esac
done
shift $(($OPTIND - 1))

if [ "$float" == "true" ]; then
  CFILE="map_binary_float.c"
  LLFILE="map_binary_float.ll"
fi

# Emit LLVM IR of test program with instrumentation
rm -f ./map_key.txt
rm -f /dev/shm/gfzidfile
../afl/bin/gfz-clang-fast -I. -S -emit-llvm "$CFILE"

# Verify LLVM IR of test program with instrumentation
opt -verify "$LLFILE" -o /dev/null

# Build test program with instrumentation
rm -f ./map_key.txt
rm -f /dev/shm/gfzidfile
../afl/bin/gfz-clang-fast -I. "$CFILE"

if [ ! -z "$run" ]; then
  if [ "$run" == "r" ]; then
    rm -f ./afl-fuzz.log && touch ./afl-fuzz.log
    ../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -G -c map_binary_cmdlines
  elif [ "$run" == "1" ]; then
    rm -f /dev/shm/fuzztest
    rm -rf /dev/shm/generated
    rm -f ./afl-fuzz.log && touch ./afl-fuzz.log
    ../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -G -g /dev/shm/fuzztest -c t1_cmdlines
    if [ "$test" == "true" ]; then
      ./test.sh -t1
    fi
  elif [ "$run" == "2" ]; then
    rm -f /dev/shm/fuzztest
    rm -rf /dev/shm/generated
    rm -f ./afl-fuzz.log && touch ./afl-fuzz.log
    ../afl/bin/afl-fuzz -i in -o out -m10000 -t5000 -G -g /dev/shm/fuzztest -c t2_cmdlines
    if [ "$test" == "true" ]; then
      ./test.sh -t2
    fi
  fi
fi