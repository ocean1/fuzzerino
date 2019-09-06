#!/bin/bash

make clean
find . -name "*.gcno" -type f -delete
find . -name "*.gcda" -type f -delete
CC="gcc --coverage" ./configure
make -j