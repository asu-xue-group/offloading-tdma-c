#!/usr/bin/env bash

# Iterate through the test files

for i in {2..10}; do
  for flag in -1 1; do
    ./tdma_knapsack "test_files/med-large/$i/input.txt" $flag 10 25
  done
done