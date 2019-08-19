#!/bin/bash

cores=$1

re='^[0-9]+$'
if ! [[ $cores =~ $re ]] ; then
   echo "usage: $0 <number of cores>" >&2; exit 1
fi

for i in `seq 1 $cores`
do
    screen -S fuzzer$i -X quit
done