# Project setup
A project using ```vuh``` consists of a standard ```C++``` code and ```SPIR-V``` precompiled
compute shader.
Connection between the two is made in the code.
It has no special requirements to ```C++``` compiler used apart from supported standard (```c++14```).
At the moment the easiest way to get a ```SPIR-V``` file is to compile ```GLSL```
compute shader using Glslang.
However it does not really matter where the ```SPIR-V``` binary comes from as long as it works
(some people enjoy writing it by hands).
```Vuh``` provides ```CMake``` helper function to do the ```GLSL``` to ```SPIR-V``` compilation.

## Using CMake
Minimal CMake project using vuh library looks like this:
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
This assumes the includes are scoped like ```#include <vuh/vuh.h>``` in the code
and ```Glslang``` compiler findable by ```CMake```.
For a complete buildable example look in ```doc/examples/```.

## Not using CMake
You will need to link ```${VUH_INSTALL_PATH}/lib/libvuh.so``` (or its equivalent for your OS)
and add ```${VUH_INSTALL_PATH}/include``` to includes search path.
Satisfying ```vuh``` public dependencies is your responsibility as well.
