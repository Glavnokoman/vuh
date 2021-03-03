find_package(Vulkan REQUIRED)

add_library(vuh ${VUH_BUILD_TYPE} device.cpp error.cpp instance.cpp utils.cpp)
target_link_libraries(vuh PUBLIC Vulkan::Vulkan)
target_include_directories(vuh
   PUBLIC
      $<INSTALL_INTERFACE:include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
add_library(vuh::vuh ALIAS vuh)

install(TARGETS vuh EXPORT VuhTargets
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
   INCLUDES DESTINATION include
)
install(DIRECTORY include/ DESTINATION include)

install(EXPORT VuhTargets
   FILE VuhTargets.cmake
   NAMESPACE vuh::
   DESTINATION lib/cmake/vuh
)

install(FILES
   ${PROJECT_SOURCE_DIR}/config/VuhConfig.cmake
   ${PROJECT_SOURCE_DIR}/config/VuhCompileShader.cmake
   DESTINATION lib/cmake/vuh
)
