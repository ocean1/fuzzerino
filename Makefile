.PHONY: FORCE all clean

test%: t% FORCE
	make -C $<

all: test1 test2

clean%: t%
	make -C $< clean

clean: clean1 clean2
