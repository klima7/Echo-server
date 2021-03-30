cmake_minimum_required(VERSION 3.17)
project(SerwerEcho)

set(CMAKE_CXX_STANDARD 14)

add_executable(SerwerEcho main.cpp)
target_link_libraries(SerwerEcho wsock32 ws2_32)