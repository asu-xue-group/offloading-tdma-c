#!/usr/bin/env bash

# Iterate through the test files

for size in "small" "medium" "big" "bigger"; do
  for num in $(seq 1 10); do
    for mode in -1 0 1; do
        ./tdma_knapsack "test_cases/${size}/${size}_${num}/input.json" $mode
    done
  done
done