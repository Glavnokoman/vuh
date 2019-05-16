cmake_minimum_required(VERSION 3.12)
project(vuh VERSION 1.99.0)

option(VUH_BUILD_BENCHMARKS "Build benchmarks for vuh library" OFF)
option(VUH_BUILD_DOCS "Build doxygen documentation for vuh" OFF)
option(VUH_BUILD_EXAMPLES "Build examples of using vuh" OFF)
option(VUH_BUILD_TESTS "Build tests for vuh library" OFF)
option(VUH_USE_EXCEPTIONS "Use exceptions to signal errors" ON)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/config)
enable_testing()

add_subdirectory(doc)
add_subdirectory(examples)
add_subdirectory(src)
add_subdirectory(test)

