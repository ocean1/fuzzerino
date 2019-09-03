#!/bin/bash

rm -rf test
find . -name "*.gcno" -type f -delete
find . -name "*.gcda" -type f -delete
make