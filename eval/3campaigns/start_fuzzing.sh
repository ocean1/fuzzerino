#!/bin/bash

cores=$1
name=$2
shift 2

re='^[0-9]+$'
if ! [[ $cores =~ $re ]] ; then
   echo "usage: $0 <number of cores> <fuzzer(s) name> <arguments to afl-fuzz>" >&2; exit 1
fi

screen -dmS $name1 afl-fuzz -M $name1 $@

for i in `seq 2 $cores`
do
    screen -dmS $name$i afl-fuzz -S $name$i $@
done