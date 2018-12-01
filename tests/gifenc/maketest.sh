EXN=$1
echo $EXN
rm -f /dev/shm/gfzidfile
AFL_NO_DEBUG=true AFL_DONT_OPTIMIZE=true PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ gfz-clang-fast -S -emit-llvm -c gifenc.c -o gifenc.ll
AFL_NO_DEBUG=true AFL_DONT_OPTIMIZE=true PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ gfz-clang-fast -S -emit-llvm -c example.c -o example.ll

PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ gfz-clang-fast -c gifenc.c -o gifenc_instr.o
PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ gfz-clang-fast -c example.c -o example_instr.o
PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ gfz-clang-fast gifenc_instr.o example_instr.o -o example_gfz -lm
