#/usr/bin/env sh
LLVM_CONFIG=llvm-config-12 CC=clang CXX=clang++ make all
LLVM_CONFIG=llvm-config-12 CC=clang CXX=clang++ make all -C llvm_mode
