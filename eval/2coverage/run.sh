#!/bin/bash

if [ ! -d ../datasets ]; then
  7z x ../datasets.7z -o.. > /dev/null
fi

for dir in `find . -mindepth 1 -maxdepth 1 -type d`; do
    cd $dir
    ./run.sh ../../datasets
    cd ..
    read -n 1 -s -r -p "Press any key to continue..."
done