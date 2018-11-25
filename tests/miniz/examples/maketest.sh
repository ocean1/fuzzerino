rm -f /dev/shm/gfzidfile
AFL_DONT_OPTIMIZE=true PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -g -c -I ../amalgamation -I ../ ../miniz.c -o miniz.o
AFL_DONT_OPTIMIZE=true PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -g -c -I ../amalgamation -I ../ ../miniz_tdef.c -o miniz_tdef.o
AFL_DONT_OPTIMIZE=true PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -g -c -I ../amalgamation -I ../ ../miniz_tinfl.c -o miniz_tinfl.o
clang-6.0 -g -c -I ../amalgamation -I ../ example5.c -o ex5.o
AFL_DONT_OPTIMIZE=true PATH=$PATH:../../../afl/bin/ AFL_PATH=../../../afl/bin/ gfz-clang-fast -g -I ../amalgamation -I ../ miniz.o miniz_tdef.o miniz_tinfl.o ex5.o -o ex5_gfz
