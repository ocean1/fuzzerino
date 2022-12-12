.PHONY: FORCE all clean generators

all: afl gfz dogen generators

afl:
	CC=clang make -C afl

gfz: afl
	cd afl && ./mk.sh

dogen:
	cd tests && ./dotests.sh

generators:
	make -C generators/gif2apng
	make -C generators/miniz
	make -C generators/stb/tests

emit:
	cd tests && ./emitall.sh

clean:
	make -C afl clean
	make -C tests clean
	make -C generators/gif2apng clean
	make -C generators/miniz clean
	make -C generators/stb/tests clean

cleanll: clean
	make -C tests cleanll
