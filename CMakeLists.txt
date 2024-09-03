cmake_minimum_required(VERSION 3.27)
project(tdma_knapsack)

set(CMAKE_CXX_STANDARD 23)

add_executable(tdma_knapsack main.cpp
        subs.hpp
        knapsack.hpp
        output.hpp)
