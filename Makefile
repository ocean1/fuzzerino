.PHONY: FORCE all clean

all: afl gfz dotests

afl:
	CC=clang-6.0 make -C afl

gfz: afl
	cd afl && ./mk.sh

dotests:
	cd tests && ./dotests.sh

emit:
	cd tests && ./emitall.sh

clean:
	make -C afl clean
	make -C tests clean

cleanall: clean
	make -C tests cleanll
