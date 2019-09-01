#!/bin/bash

for dir in `find . -mindepth 1 -maxdepth 1 -type d`; do
    cd $dir
    ./mk.sh
    cd ..
done