clang-6.0 -g -c -I ../amalgamation -I ../ ../miniz.c -o miniz.o -fxray-instrument -fxray-instruction-threshold=1
clang-6.0 -g -c -I ../amalgamation -I ../ ../miniz_tdef.c -o miniz_tdef.o -fxray-instrument -fxray-instruction-threshold=1
clang-6.0 -g -c -I ../amalgamation -I ../ ../miniz_tinfl.c -o miniz_tinfl.o -fxray-instrument -fxray-instruction-threshold=1
clang-6.0 -g -c -I ../amalgamation -I ../ example5.c -o ex5_xray.o -fxray-instrument -fxray-instruction-threshold=1
clang++-6.0 -g -fxray-instrument -I ../amalgamation -I ../ miniz.o miniz_tdef.o miniz_tinfl.o ex5_xray.o -o ex5_xray

llvm-xray-6.0 convert -output-format=yaml xray-log.ex5_xray.7uPoJ7  -symbolize -instr_map ex5_xray > ex5_trace.yaml

