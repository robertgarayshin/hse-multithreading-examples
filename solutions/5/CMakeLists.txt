cmake_minimum_required(VERSION 3.30)
project(5)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

add_executable(producer producer.cpp)
add_executable(consumer consumer.cpp)

target_link_libraries(producer Threads::Threads)
target_link_libraries(consumer Threads::Threads)

if(UNIX AND NOT APPLE)
    target_link_libraries(producer rt)
    target_link_libraries(consumer rt)
endif()
