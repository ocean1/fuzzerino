.PHONY: FORCE all clean generators parsers

all: gfz dogen generators cov parsers

gfz: afl
	cd afl && ./mk.sh

dogen:
	cd tests && ./dotests.sh

generators:
	make -C generators/gif2apng
	make -C generators/miniz
	make -C generators/stb/tests

cov:
	cd afl-cov && ./mk.sh

parsers:
	cd parsers/libpng && CC=../../afl-cov/afl-clang-fast ./configure --disable-shared && make
	cd parsers/libpng/contrib/libtests && ../../../../afl-cov/afl-clang-fast ./readpng.c -lm -lz ../../.libs/libpng16.a -o readpng
	make -C parsers/picopng

clean:
	make -C afl clean
	make -C tests clean
	make -C generators/gif2apng clean
	make -C generators/miniz clean
	make -C generators/stb/tests clean
	make -C afl-cov clean
	make -C parsers/libpng clean
	make -C parsers/picopng clean

emit:
	cd tests && ./emitall.sh

cleanll: clean
	make -C tests cleanll