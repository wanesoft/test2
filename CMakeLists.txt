cmake_minimum_required(VERSION 3.16)
project(untitled)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(libevent)

add_executable(untitled main.cpp)

include_directories(libevent/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/libevent/include)

link_directories(${CMAKE_CURRENT_BINARY_DIR}/libevent/bin)

target_link_libraries(untitled event)