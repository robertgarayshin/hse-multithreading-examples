cmake_minimum_required(VERSION 3.30)
project(2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

add_executable(apply_test test.cpp)
target_link_libraries(apply_test PRIVATE GTest::gtest GTest::gtest_main)

add_executable(apply_benchmark benchmark.cpp)
target_link_libraries(apply_benchmark PRIVATE benchmark::benchmark benchmark::benchmark_main)
