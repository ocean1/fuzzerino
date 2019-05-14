.PHONY: FORCE all clean generators

all: afl gfz dogen

afl:
	CC=clang-6.0 make -C afl

gfz: afl
	cd afl && ./mk.sh

dogen:
	cd tests && ./dotests.sh

generators:
	cd tests && ./compileall.sh

emit:
	cd tests && ./emitall.sh

clean:
	make -C afl clean
	make -C tests clean

cleanll: clean
	make -C tests cleanll
