cmake_minimum_required(VERSION 2.8)

project(design_patterns)

message(STATUS "CMake C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMake C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20 -g -Wall -Wextra")

add_subdirectory(google_benchmark benchmark EXCLUDE_FROM_ALL)
if(NOT DEFINED BENCHMARK_ROOT)
    set(BENCHMARK_ROOT ${CMAKE_SOURCE_DIR}/google_benchmark)
endif()

include_directories(${BENCHMARK_ROOT}/include)
link_directories(${BENCHMARK_ROOT}/lib)

add_subdirectory(branch_reduction)
add_subdirectory(prefetching)
add_subdirectory(cache_warming)
add_subdirectory(simd)
add_subdirectory(crtp)
add_subdirectory(lock_free)
add_subdirectory(short_circuiting)
add_subdirectory(test)
