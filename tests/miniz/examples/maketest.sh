rm -f /dev/shm/gfzidfile
AFL_DONT_OPTIMIZE=true PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -g -c -I ../amalgamation -I ../ ../miniz.c -o miniz_instr.o
AFL_DONT_OPTIMIZE=true PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -g -c -I ../amalgamation -I ../ ../miniz_tdef.c -o miniz_tdef_instr.o
AFL_DONT_OPTIMIZE=true PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -g -c -I ../amalgamation -I ../ ../miniz_tinfl.c -o miniz_tinfl_instr.o
clang-6.0 -g -c -I ../amalgamation -I ../ example5.c -o ex5.o
AFL_DONT_OPTIMIZE=true PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -g -I ../amalgamation -I ../ miniz_instr.o miniz_tdef_instr.o miniz_tinfl_instr.o ex5.o -o ex5_gfz
