cmake_minimum_required(VERSION 3.8)
project(vuh_example_saxpy_async)

find_package(Vuh REQUIRED)

vuh_compile_shader(example_shader_saxpy_async
   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/saxpy.comp
   TARGET ${CMAKE_CURRENT_BINARY_DIR}/saxpy.spv
)
add_executable(vuh_example_saxpy_async main.cpp)
target_link_libraries(vuh_example_saxpy_async PUBLIC vuh::vuh)
add_dependencies(vuh_example_saxpy_async example_shader_saxpy_async)
