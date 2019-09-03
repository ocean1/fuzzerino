#!/bin/bash

make clean
find . -name "*.gcno" -type f -delete
find . -name "*.gcda" -type f -delete
./configure CFLAGS='--coverage' CXXFLAGS='--coverage'
make -j
cd contrib/libtests
gcc readpng.c ../../.libs/libpng16.a -lm -lz --coverage -o readpng