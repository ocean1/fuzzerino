EXN=main
echo $EXN
rm -f /dev/shm/gfzidfile
PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ gfz-clang-fast -g tj.c -o tj_gfz -lm
PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ gfz-clang-fast -S -emit-llvm -g tj.c -o tj.ll -lm
#miniz_zip_instr.o 
