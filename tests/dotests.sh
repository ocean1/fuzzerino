AFL_DONT_OPTIMIZE=1 GFZ_WHITE_LIST=1 PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ CC=gfz-clang-fast make clean all
make do_tests