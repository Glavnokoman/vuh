# Project setup
A project using ```vuh``` consists of a standard ```C++``` code and ```SPIR-V``` precompiled
compute shader.
Connection between the two is made in the ```C++``` code.
At the moment the easiest way to get a ```SPIR-V``` file is to compile a
compute shader using [Glslang](https://github.com/KhronosGroup/glslang).
At that the shader should be written in a [vulkan dialect](https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt)
of ```GLSL``` language.
```Vuh``` provides ```CMake``` helpers to handle the ```GLSL``` to ```SPIR-V``` compilation.

## Using CMake
Minimal CMake project using vuh library and including shader compilation looks like this:
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
and ```Glslang``` compiler findable by ```CMake```. For a complete buildable example look in ```doc/examples/```.

## Not using CMake
You will need to link ```${VUH_INSTALL_PATH}/lib/libvuh.so``` (or its equivalent for your OS)
and add ```${VUH_INSTALL_PATH}/include``` to includes search path.
Satisfying ```vuh``` public dependencies is your responsibility as well.
