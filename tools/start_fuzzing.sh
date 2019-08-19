#!/bin/bash

cores=$1
shift

re='^[0-9]+$'
if ! [[ $cores =~ $re ]] ; then
   echo "usage: $0 <number of cores> <arguments to afl-fuzz>" >&2; exit 1
fi

screen -dmS fuzzer1 afl-fuzz -M fuzzer1 $@

for i in `seq 2 $cores`
do
    screen -dmS fuzzer$i afl-fuzz -S fuzzer$i $@
done