#!/bin/bash

if [ $# -ge 1 ] && [ $1 == "-t1" ]
  then
    if [ $# -ge 2 ] && [ $2 == "--dry" ]
      then
        for i in ./out/generated/dry*; do
          echo === $i ===
          ../tests/t1/ptest $i
          echo
        done
    else
      for i in ./out/generated/*; do
        echo === $i ===
        ../tests/t1/ptest $i
        echo
      done
    fi
fi

if [ $# -ge 1 ] && [ $1 == "-t2" ]
  then
    if [ $# -ge 2 ] && [ $2 == "--dry" ]
      then
        for i in ./out/generated/dry*; do
          echo === $i ===
          ../tests/t2/ptest $i
          echo
        done
    else
      for i in ./out/generated/*; do
        echo === $i ===
        ../tests/t2/ptest $i
        echo
      done
    fi
fi