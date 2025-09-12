cmake_minimum_required(VERSION 3.5)

project(disruptor)

message(STATUS "CMake C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMake C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

# Find GoogleTest package
find_package(GTest REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++20 -g -Wall -Wextra")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

# Add your test executable
add_executable(gtest_disruptor gtest_disruptor.cpp)

target_link_libraries(gtest_disruptor
    GTest::gtest
    pthread
)
