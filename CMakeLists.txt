cmake_minimum_required(VERSION 3.27)
project(tdma_knapsack)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
            "Choose the type of build." FORCE)
endif()

# Add march flag to Release configuration (not portable, faster)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
endif()

include(FetchContent)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(tdma_knapsack main.cpp
        subs.hpp
        knapsack.hpp
        output.hpp
        structs.h
        global.h
        graph.hpp)

target_link_libraries(tdma_knapsack PRIVATE nlohmann_json::nlohmann_json)