.PHONY: FORCE all clean generators parsers

all: gfz dogen generators cov parsers

gfz: afl
	cd afl && ./mk.sh

dogen:
	cd tests && ./dotests.sh

generators:
	make -C generators/gif2apng
	rm -rf /dev/shm/gfzidfile
	make -C generators/libpng-wpng
	make -C generators/libpng-wpng/contrib/gregbook
	make -C generators/miniz
	make -C generators/stb/tests

cov:
	cd afl-cov && ./mk.sh

parsers:
	cd parsers/libpng && CC=../../afl-cov/afl-clang-fast ./configure --disable-shared && make
	cd parsers/libpng/contrib/libtests && ../../../../afl-cov/afl-clang-fast ./readpng.c -lm -lz ../../.libs/libpng16.a -o readpng
	make -C parsers/picopng
	make -C parsers/lodepng

clean:
	make -C afl clean
	make -C tests clean
	make -C generators/gif2apng clean
	make -C generators/libpng-wpng clean
	make -C generators/libpng-wpng/contrib/gregbook clean
	make -C generators/miniz clean
	make -C generators/stb/tests clean
	make -C afl-cov clean
	make -C parsers/libpng clean
	rm -rf parsers/libpng/contrib/libtests/readpng
	make -C parsers/picopng clean
	make -C parsers/lodepng clean

emit:
	cd tests && ./emitall.sh

cleanll: clean
	make -C tests cleanll