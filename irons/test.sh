#!/bin/bash

rm -rf ./out/generated/* || true
mv /dev/shm/generated ./out/ || true

if [ $# -eq 1 ] && [ $1 == "-t1" ]
  then
    for i in ./out/generated/*; do
        echo === $i ===
        ../../tests/t1/ptest $i
        echo
    done
fi

if [ $# -eq 1 ] && [ $1 == "-t2" ]
  then
    for i in ./out/generated/*; do
        echo === $i ===
        ../../tests/t2/ptest $i
        echo
    done
fi