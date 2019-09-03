#!/bin/bash

set -x

for dir in `find . -mindepth 1 -maxdepth 1 -type d`; do
    cd $dir
    ./run.sh ../../datasets
    cd ..
    read -n 1 -s -r -p "Press any key to continue..."
done