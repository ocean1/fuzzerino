.PHONY: FORCE all clean tests afl fuzzerino dogen generators emit

LLVM_CONFIG=llvm-config-6.0
CC=clang-6.0
AFL_DONT_OPTIMIZE?=true

export CC
export LLVM_CONFIG
export AFL_DONT_OPTIMIZE

all: afl dogen

tests: emit
	make -C tests

afl:
	make -C afl

gfz: afl
	cd afl && ./mk.sh

clean:
	make -C afl clean
	make -C tests clean

cleanll: clean
	make -C tests cleanll
