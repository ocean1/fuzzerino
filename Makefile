.PHONY: FORCE all clean generators

all: afl gfz dogen

afl:
	CC=clang-6.0 make -C afl

gfz: afl
	cd afl && ./mk.sh

dogen:
	cd generator && ./dotests.sh

generators:
	cd generators && ./compileall.sh

emit:
	cd generators && ./emitall.sh

clean:
	make -C afl clean
	make -C generators clean

cleanall: clean
	make -C generator cleanll
