#!/bin/bash

make -C cbuild clean
find . -name "*.gcno" -type f -delete
find . -name "*.gcda" -type f -delete
rm -rf cbuild
mkdir cbuild
cd cbuild
cmake -E env CFLAGS="--coverage" cmake ..
make
cd ..
gcc ok_png_cov.c cbuild/libokfileformats.a --coverage -o ok_png_cov