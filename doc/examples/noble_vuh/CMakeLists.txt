cmake_minimum_required(VERSION 3.8)
project(vuh_NobleVulkan)

find_package(Vuh REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast -march=native -mtune=native")

vuh_compile_shader(NobleVulkan_kernel_clr
   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/kernel_clr.comp
   TARGET ${CMAKE_CURRENT_BINARY_DIR}/kernel_clr.spv
)

vuh_compile_shader(NobleVulkan_kernel1
   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/kernel_noble_1.comp
   TARGET ${CMAKE_CURRENT_BINARY_DIR}/kernel_noble_1.spv
)

vuh_compile_shader(NobleVulkan_kernel2
   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/kernel_noble_2.comp
   TARGET ${CMAKE_CURRENT_BINARY_DIR}/kernel_noble_2.spv
)

add_executable(vuh_NobleVulkan main.cpp)
target_link_libraries(vuh_NobleVulkan PUBLIC vuh::vuh)
add_dependencies(vuh_NobleVulkan NobleVulkan_kernel_clr NobleVulkan_kernel1 NobleVulkan_kernel2)
