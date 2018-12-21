EXN=$1

rm -f xray-log.*

clang-6.0 -g -c -I ../amalgamation -I ../ ../miniz.c -o miniz.o -fxray-instrument -fxray-instruction-threshold=1
clang-6.0 -g -c -I ../amalgamation -I ../ ../miniz_tdef.c -o miniz_tdef.o -fxray-instrument -fxray-instruction-threshold=1
clang-6.0 -g -c -I ../amalgamation -I ../ ../miniz_tinfl.c -o miniz_tinfl.o -fxray-instrument -fxray-instruction-threshold=1
clang-6.0 -g -c -I ../amalgamation -I ../ ../miniz_zip.c -o miniz_zip.o -fxray-instrument -fxray-instruction-threshold=1
clang-6.0 -g -c -I ../amalgamation -I ../ example${EXN}.c -o ex${EXN}_xray.o -fxray-instrument -fxray-instruction-threshold=1
clang++-6.0 -g -fxray-instrument -I ../amalgamation -I ../ miniz.o miniz_zip.o miniz_tdef.o miniz_tinfl.o ex${EXN}_xray.o -o ex${EXN}_xray

XRAY_OPTIONS="patch_premain=true xray_mode=xray-basic format=yaml"  ./ex${EXN}_xray c seed/gtinrnd /dev/shm/fuzztest
llvm-xray-6.0 convert -output-format=yaml xray-log.*  -symbolize -instr_map ex${EXN}_xray > ex${EXN}_trace.yaml

