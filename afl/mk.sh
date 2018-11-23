#/usr/bin/env sh
LLVM_CONFIG=llvm-config-6.0 CC=clang-6.0 CXX=g++ make all
LLVM_CONFIG=llvm-config-6.0 CC=clang-6.0 CXX=g++ make all -C llvm_mode
