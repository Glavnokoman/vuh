find_package(Vulkan REQUIRED)

add_library(vuh SHARED device.cpp instance.cpp utils.cpp)
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
   ${PROJECT_SOURCE_DIR}/config/CompileShader.cmake
   DESTINATION lib/cmake/vuh
)

# https://github.com/pabloariasal/modern-cmake-sample/blob/master/libjsonutils/CMakeLists.txt

# configure_package_config_file(${CMAKE_CURRENT_LIST_DIR}/cmake/JSONUtilsConfig.cmake.in
#     ${CMAKE_CURRENT_BINARY_DIR}/JSONUtilsConfig.cmake
#     INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
# )
#
# #Install the config, configversion and custom find modules
# install(FILES
#     ${CMAKE_CURRENT_LIST_DIR}/cmake/FindRapidJSON.cmake
#     ${CMAKE_CURRENT_BINARY_DIR}/JSONUtilsConfig.cmake
#     ${CMAKE_CURRENT_BINARY_DIR}/JSONUtilsConfigVersion.cmake
#     DESTINATION ${INSTALL_CONFIGDIR}
# )
