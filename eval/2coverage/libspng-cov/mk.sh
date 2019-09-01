#!/bin/bash

make -C cbuild clean
find . -name "*.gcno" -type f -delete
find . -name "*.gcda" -type f -delete
rm -rf cbuild
mkdir cbuild
cd cbuild
cmake -E env CFLAGS="--coverage" cmake ..
make
cd ../examples
gcc example.c ../cbuild/libspng_static.a -I.. -lm -lz --coverage -o example