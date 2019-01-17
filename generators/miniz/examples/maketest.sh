EXN=$1
echo $EXN
rm -f /dev/shm/gfzidfile
CFLAGS="-g -I ../amalgamation -I ../"

PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast $CFLAGS -c ../miniz.c -o miniz_instr.o
PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast $CFLAGS -c ../miniz_tdef.c -o miniz_tdef_instr.o
PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast $CFLAGS -c ../miniz_tinfl.c -o miniz_tinfl_instr.o
PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast $CFLAGS -c ../miniz_zip.c -o miniz_zip_instr.o
clang-6.0 -g -c -I ../amalgamation -I ../ -c example${EXN}.c -o ex${EXN}.o
PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast $CFLAGS miniz_instr.o miniz_tdef_instr.o miniz_zip_instr.o miniz_tinfl_instr.o ex${EXN}.o -o ex${EXN}_gfz -lm
#miniz_zip_instr.o 
