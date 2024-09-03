#!/usr/bin/env bash

# Iterate through the test files

for size in "small" "medium" "med-large" "large"; do
  for num in $(seq 1 10); do
    for mode in -1 0 1; do
      for T in 10 20; do
        ./tdma_knapsack "test_files/${size}/${num}/input.txt" $mode $T
      done
    done
  done
done