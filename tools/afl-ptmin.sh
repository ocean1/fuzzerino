#!/bin/bash

cores=$1
inputdir=$2
outputdir=$3
parser=$4

if  [[ $# -lt 4 ]] ; then
   echo "usage: $0 <number of cores> <input dir> <output dir> <path to fuzzed app>" >&2; exit 1
fi

mkdir $outputdir
pids=""
total=`ls $inputdir | wc -l`

for k in `seq 1 $cores $total`
do
  for i in `seq 0 $(expr $cores - 1)`
  do
    file=`ls -Sr $inputdir | sed $(expr $i + $k)"q;d"`
    echo $file
    afl-tmin -i $inputdir/$file -o $outputdir/$file -- $parser &
  done

  wait
done