# This makefile only makes the unit test, benchmark and pngdetail and showpng
# utilities. It does not make the PNG codec itself as shared or static library.
# That is because:
# LodePNG itself has only 1 source file (lodepng.cpp, can be renamed to
# lodepng.c) and is intended to be included as source file in other projects and
# their build system directly.


override CFLAGS := -W -Wall -Wextra -ansi -pedantic -O3 -Wno-unused-function $(CFLAGS)
override CXXFLAGS := -W -Wall -Wextra -ansi -pedantic -O3 $(CXXFLAGS)

all: pngdetail

%.o: %.cpp
	@mkdir -p `dirname $@`
	../../afl-cov/afl-clang-fast++ -I ./ $(CXXFLAGS) -c $< -o $@

pngdetail: lodepng.o lodepng_util.o pngdetail.o
	../../afl-cov/afl-clang-fast++ $^ $(CXXFLAGS) -o $@

clean:
	rm -f pngdetail lodepng.o lodepng_util.o pngdetail.o