# Project setup
## Using CMake
minimal CMake project using vuh library looks like this:
```cmake
cmake_minimum_required (VERSION 3.8)
project(vuh_example)

find_package(Vuh REQUIRED)

vuh_compile_shader(example_shader
   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/example.comp
   TARGET ${CMAKE_CURRENT_BINARY_DIR}/example.spv
)
add_executable(vuh_example main.cpp)
target_link_libraries(vuh_example PUBLIC vuh::vuh)
add_dependencies(vuh_example example_shader)
```
This assumes the includes are scoped like ```#include <vuh/vuh.h>```.
For a complete buildable example look at ```doc/examples/```.

## Not using CMake
You will need to link ```${VUH_INSTALL_PATH}/lib/libvuh.so``` (or its equivalent for your OS)
and add ```${VUH_INSTALL_PATH}/include``` to includes search path to use scoped includes like
```#include <vuh/vuh.h>```. Satisfying ```Vuh``` public dependencies is your responsibility
as well.
