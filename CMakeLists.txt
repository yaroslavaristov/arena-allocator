cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_COMPILER g++-14)
project(arena_allocator)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(arena src/main.cpp)
target_include_directories(arena PRIVATE include)

add_executable(stress_test tests/stress_test.cpp)
target_include_directories(stress_test PRIVATE include)

find_package(benchmark)

add_executable(bench benchmarks/bench.cpp)
target_include_directories(bench PRIVATE include)
target_link_libraries(bench benchmark::benchmark)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -funroll-loops -Wextra -Wall -Werror -DNDEBUG")
