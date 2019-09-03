#!/bin/bash

out=$1
parser=$2

if  [[ $# -lt 2 ]] ; then
   echo "usage: $0 <afl-fuzz output dir> <path to fuzzed app>" >&2; exit 1
fi

mkdir $out/all
cp $out/*/queue/* $out/all 2> /dev/null
afl-cmin -i $out/all -o $out/queue -- $parser
rm -rf $out/all