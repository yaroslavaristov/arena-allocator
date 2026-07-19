cmake_minimum_required(VERSION 3.20)
project(arena_allocator)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(arena src/main.cpp)
target_include_directories(arena PRIVATE include)

add_executable(stress_test tests/stress_test.cpp)
target_include_directories(stress_test PRIVATE include)
