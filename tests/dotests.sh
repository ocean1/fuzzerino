AFL_DONT_OPTIMIZE=true PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ CC=gfz-clang-fast make clean all
make do_tests
