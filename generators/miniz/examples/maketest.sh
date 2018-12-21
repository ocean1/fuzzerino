EXN=$1
echo $EXN
rm -f /dev/shm/gfzidfile
PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -O0 -g -c -I ../amalgamation -I ../ ../miniz.c -o miniz_instr.o
PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -O0 -g -c -I ../amalgamation -I ../ ../miniz_tdef.c -o miniz_tdef_instr.o
PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -O0 -g -c -I ../amalgamation -I ../ ../miniz_tinfl.c -o miniz_tinfl_instr.o
PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -O0 -g -c -I ../amalgamation -I ../ ../miniz_zip.c -o miniz_zip_instr.o
clang-6.0 -g -c -I ../amalgamation -I ../ example${EXN}.c -o ex${EXN}.o
PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -g -I ../amalgamation -I ../ miniz_instr.o miniz_tdef_instr.o miniz_zip_instr.o miniz_tinfl_instr.o ex${EXN}.o -o ex${EXN}_gfz -lm
#miniz_zip_instr.o 
