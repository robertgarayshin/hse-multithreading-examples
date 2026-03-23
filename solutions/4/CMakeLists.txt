cmake_minimum_required(VERSION 3.20)
project(4)

set(CMAKE_CXX_STANDARD 20)

add_executable(4 main.cpp mutex.cpp)
target_include_directories(4 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
