find_package(Vulkan 1.0 REQUIRED)

list(APPEND VUH_PUBLIC_DEFINES $<$<NOT:$<BOOL:${VUH_USE_EXCEPTIONS}>>:VUH_NO_EXCEPTIONS>)

add_library(vuh)
target_sources(vuh
	PRIVATE 
		error_noexcept.cpp
		device.cpp
		instance.cpp
		physicalDevice.cpp
		utils.cpp
)
target_link_libraries(vuh PUBLIC Vulkan::Vulkan)
target_compile_features(vuh PUBLIC cxx_std_17)
target_compile_definitions(vuh PUBLIC ${VUH_PUBLIC_DEFINES})
target_include_directories(vuh
   PUBLIC
      $<INSTALL_INTERFACE:include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
   PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include/vuh
)
add_library(vuh::vuh ALIAS vuh)

################################### install ###################################

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
