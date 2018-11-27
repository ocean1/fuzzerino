rm -f xray-log.*

clang-6.0 -c -g gifenc.c -o gifenc.o  -fxray-instrument -fxray-instruction-threshold=1
clang-6.0 -c example.c -o example.o -fxray-instrument -fxray-instruction-threshold=1
clang++-6.0 -fxray-instrument gifenc.o example.o -o example_xray

XRAY_OPTIONS="patch_premain=true xray_mode=xray-basic format=yaml"  ./example_xray
llvm-xray-6.0 convert -output-format=yaml xray-log.*  -symbolize -instr_map example_xray > example_trace.yaml

