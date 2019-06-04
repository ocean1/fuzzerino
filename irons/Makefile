.PHONY: FORCE all 

AFL_DONT_OPTIMIZE = true

CC = ../afl/bin/gfz-clang-fast
CFLAGS = -emit-llvm -S #-Xclang -disable-O0-optnone

LD = ld
LDFLAGS = -z relro --hash-style=gnu --eh-frame-hdr -m elf_x86_64 -dynamic-linker /lib64/ld-linux-x86-64.so.2 /usr/bin/../lib/gcc/x86_64-linux-gnu/7.4.0/../../../x86_64-linux-gnu/crt1.o /usr/bin/../lib/gcc/x86_64-linux-gnu/7.4.0/../../../x86_64-linux-gnu/crti.o /usr/bin/../lib/gcc/x86_64-linux-gnu/7.4.0/crtbegin.o -L/usr/bin/../lib/gcc/x86_64-linux-gnu/7.4.0 -L/usr/bin/../lib/gcc/x86_64-linux-gnu/7.4.0/../../../x86_64-linux-gnu -L/lib/x86_64-linux-gnu -L/lib/../lib64 -L/usr/lib/x86_64-linux-gnu -L/usr/bin/../lib/gcc/x86_64-linux-gnu/7.4.0/../../.. -L/usr/lib/llvm-6.0/bin/../lib -L/lib -L/usr/lib -lgcc --as-needed -lgcc_s --no-as-needed -lc -lgcc --as-needed -lgcc_s --no-as-needed /usr/bin/../lib/gcc/x86_64-linux-gnu/7.4.0/crtend.o /usr/bin/../lib/gcc/x86_64-linux-gnu/7.4.0/../../../x86_64-linux-gnu/crtn.o 

FILENAME = test

all: ...

emit: $(FILENAME).c
	$(CC) $(CFLAGS) -o $(FILENAME).ll $<

verify: ...
	...

as: $(FILENAME).ll
	opt

link: $(FILENAME)
	...

clean: ...
	...

emitllvm: g.ll

%.ll:
	@rm -f /dev/shm/gfzidfile
	$(CC) $(CFLAGS) $(LLVMFLAGS) $*.c -o $@

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

clean:
	make -C afl clean
	make -C tests clean

cleanll: clean
	make -C tests cleanll

run:
	GFZ_NUM_ITER=10000 AFL_SKIP_CPUFREQ=1 afl/bin/afl-fuzz -i irons/in -o irons/out -m10000 -t5000 -G tests/t1/gtest
