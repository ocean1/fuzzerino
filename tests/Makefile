.PHONY: FORCE all clean

LLVM_CONFIG=llvm-config-6.0
CC=clang-6.0
AFL_DONT_OPTIMIZE?=true
DEBUG ?= 0

export CC
export LLVM_CONFIG
export AFL_DONT_OPTIMIZE
export DEBUG


test%: t% FORCE
	make -C $<

all: test1 test2 emit_all all_verify

clean%: t%
	make -C $< clean

clean: clean1 clean2

cleanll%: t%
	make -C $< cleanll

cleanll: cleanll1 cleanll2

extest%: t% FORCE
	@echo "testing t$*"
	@$</gtest
	@$</ptest
	@echo ""

do_tests: extest1 extest2

emit%: t%
	make -C $< emitllvm

emit_all: emit1 emit2

verif%:
	make -C t$* verify

all_verify: verif1 verif2
