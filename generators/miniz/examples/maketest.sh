EXN=$1
echo $EXN

CFLAGS="-g -I ../amalgamation -I ../"

rm -rf /dev/shm/gfzidfile

../../../afl/bin/gfz-clang-fast $CFLAGS -c ../miniz.c -o miniz_instr.o
../../../afl/bin/gfz-clang-fast $CFLAGS -c ../miniz_tdef.c -o miniz_tdef_instr.o
../../../afl/bin/gfz-clang-fast $CFLAGS -c ../miniz_tinfl.c -o miniz_tinfl_instr.o
../../../afl/bin/gfz-clang-fast $CFLAGS -c ../miniz_zip.c -o miniz_zip_instr.o
clang-6.0 -g -c -I ../amalgamation -I ../ -c example${EXN}.c -o ex${EXN}.o
../../../afl/bin/gfz-clang-fast $CFLAGS miniz_instr.o miniz_tdef_instr.o miniz_zip_instr.o miniz_tinfl_instr.o ex${EXN}.o -o ex${EXN}_gfz -lm