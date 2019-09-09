#!/bin/bash

rm -rf unittest benchmark pngdetail showpng lodepng_unittest.o lodepng_benchmark.o lodepng.o lodepng_util.o pngdetail.o examples/example_sdl.o
find . -name "*.gcno" -type f -delete
find . -name "*.gcda" -type f -delete
make