cmake_minimum_required(VERSION 3.27)
project(tdma_knapsack)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(tdma_knapsack main.cpp
        subs.hpp
        knapsack.hpp
        output.hpp
        graph.hpp)

target_link_libraries(tdma_knapsack PRIVATE nlohmann_json::nlohmann_json)