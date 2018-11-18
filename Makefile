.PHONY: FORCE all clean

DEBUG = 1

export DEBUG

test%: t% FORCE
	make -C $<

all: test1 test2

clean%: t%
	make -C $< clean

clean: clean1 clean2

extest%: t% FORCE
	$</g
	$</p

do_tests: extest1 extest2
