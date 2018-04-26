find_package(Vulkan REQUIRED)

add_library(vuh SHARED)
target_sources(vuh PRIVATE
   device.cpp
   helpers.cpp
   instance.cpp
   pipe.cpp
   string_view.cpp
)
target_link_libraries(vuh PUBLIC Vulkan::Vulkan)
target_include_directories(vuh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
