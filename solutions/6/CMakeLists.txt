cmake_minimum_required(VERSION 3.30)
project(6)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

add_executable(6 main.cpp)
target_link_libraries(6 PRIVATE Threads::Threads)
