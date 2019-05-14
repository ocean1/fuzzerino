.PHONY: FORCE all clean tests afl fuzzerino dogen generators emit

all: afl dogen

#fuzzerino: afl
#	cd afl && ./mk.sh

afl:
	LLVM_CONFIG=llvm-config-6.0 CC=clang-6.0 make -C afl

emit:
	cd tests && ./emitall.sh

tests: emit
	make -C tests
#	cd tests && ./compileall.sh

clean:
	make -C afl clean
	make -C generators clean

cleanall: clean
	make -C generator cleanll
