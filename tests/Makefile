.PHONY: FORCE all clean

DEBUG ?= 0

export DEBUG

test%: t% FORCE
	make -C $<

all: test1 test2

clean%: t%
	make -C $< clean

clean: clean1 clean2

extest%: t% FORCE
	@echo "testing t$*"
	@$</gtest
	@$</ptest
	@echo ""

do_tests: extest1 extest2

emit%: t%
	make -C $< emitllvm

emit_all: emit1 emit2
