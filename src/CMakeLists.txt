find_package(Vulkan REQUIRED)

add_library(vuh SHARED instance.cpp)
target_sources(vuh PRIVATE
   device.cpp
   helpers.cpp
   pipe.cpp
)
target_link_libraries(vuh PUBLIC Vulkan::Vulkan)
target_include_directories(vuh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
