EXN=$1
echo $EXN
rm -f /dev/shm/gfzidfile
PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ gfz-clang-fast -g -c gifenc.c -o gifenc_instr.o
PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ gfz-clang-fast -g -c example.c -o example_instr.o
PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ gfz-clang-fast -g gifenc_instr.o example_instr.o -o example_gfz -lm
