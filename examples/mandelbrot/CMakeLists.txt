cmake_minimum_required(VERSION 3.8)
project(vuh_example_mandelbrot)

find_package(Vuh REQUIRED)

vuh_compile_shader(example_shader_mandelbrot
   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/mandelbrot.comp
   TARGET ${CMAKE_CURRENT_BINARY_DIR}/mandelbrot.spv
)
add_executable(vuh_example_mandelbrot main.cpp)
target_link_libraries(vuh_example_mandelbrot PUBLIC vuh::vuh)
add_dependencies(vuh_example_mandelbrot example_shader_mandelbrot)
