#!/usr/bin/env bash
# instrumented
AFL_DONT_OPTIMIZE=true PATH=$PATH:../../afl/bin/ AFL_PATH=../../afl/bin/ CC=gfz-clang-fast make cleanll emit_all

for f in `find ./ -iname *.ll`; do
    filename=$(basename "$f" .ll)
    dirname=$(dirname $f)
    mv -f $f ${dirname}/${filename}_inst.ll
done

# non-instrumented
CC=clang-6.0 make emit_all
