#!/usr/bin/env bash

# Iterate through the test files

for total in 10 20 30 40; do
  for T in 10 20; do
    ./tdma_knapsack "test_files/med-large/1/input.txt" 0 $T $total
  done
done