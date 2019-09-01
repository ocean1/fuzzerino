#!/bin/bash

make clean
find . -name "*.gcno" -type f -delete
find . -name "*.gcda" -type f -delete
./configure CFLAGS='--coverage' CXXFLAGS='--coverage'
make -j