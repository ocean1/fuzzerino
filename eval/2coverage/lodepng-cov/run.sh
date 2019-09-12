#!/bin/bash

if  [[ $# -lt 1 ]] ; then
   echo "usage: $0 <seeds folder>" >&2; exit 1
fi

rm -rf cov*

for seedfolder in $1/*; do
    lcov --directory . --zerocounters
    for seed in $seedfolder/*; do
        ./pngdetail -z -c -f -b -B -7 -r --mode=hex -l -i -p -h $seed > /dev/null 2> /dev/null
    done
    lcov --directory . --capture --output-file cov-${seedfolder##*/}.info
    genhtml cov-${seedfolder##*/}.info -o cov-${seedfolder##*/}
    firefox cov-${seedfolder##*/}/index.html&
done
