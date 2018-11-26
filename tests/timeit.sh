#!/usr/bin/env zsh

start=$(date +%s.%N); \
  ${@}; \
  dur=$(echo "$(date +%s.%N) - $start" | bc); \
  printf "Execution time: %.6f seconds\n" $dur
