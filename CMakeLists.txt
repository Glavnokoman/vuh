cmake_minimum_required (VERSION 3.8)
project(vuh)

option(VUH_BUILD_TESTS "Build tests for vuh library" OFF)
option(VUH_BUILD_BENCHMARKS "Build benchmarks for vuh library" OFF)

set(CMAKE_CXX_STANDARD 14)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/config)
enable_testing()

add_subdirectory(src)
add_subdirectory(test)
