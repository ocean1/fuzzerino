#!/bin/bash

out=$1

if  [[ $# -lt 1 ]] ; then
   echo "usage: $0 <afl-fuzz output dir>" >&2; exit 1
fi

mkdir tmp

for dir in $out/*
do
    afl-plot $dir tmp/${dir##*/} > /dev/null
done

python3 merge_graphs.py tmp

mv tmp $out
mv $out/tmp $out/graphs