.PHONY: FORCE all clean tests afl fuzzerino dogen generators emit

all: afl dogen

tests: emit
	make -C tests

afl:
	LLVM_CONFIG=llvm-config-6.0 CC=clang-6.0 make -C afl

gfz: afl
	cd afl && ./mk.sh

clean:
	make -C afl clean
	make -C tests clean

cleanll: clean
	make -C tests cleanll
