#!/bin/bash

rm -rf picopng
find . -name "*.gcno" -type f -delete
find . -name "*.gcda" -type f -delete
g++ picopng.cpp --coverage -o picopng