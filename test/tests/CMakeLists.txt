if(NOT VUH_BUILD_TESTS)
	return()
endif()

find_package(Catch2 REQUIRED)
function(add_catch_test arg_test_name)
	add_executable(${arg_test_name} ${ARGN})
	target_link_libraries(${arg_test_name} PRIVATE Catch2::Catch2 vuh::vuh)
	target_compile_features(${arg_test_name} PUBLIC cxx_std_17)
	target_include_directories( ${arg_test_name} PRIVATE ${PROJECT_SOURCE_DIR}/src )
	add_test(NAME ${arg_test_name} COMMAND ${arg_test_name} )
#	add_dependencies(${arg_test_name} link_shaders_test)
endfunction()

add_catch_test(core-test
	 device_t.cpp
	 instance_t.cpp 
	 physicalDevice_t.cpp
)

add_catch_test(buffer-test buffer_t.cpp)
add_catch_test(buffer-async-host-test ./buffer_async_host_t.cpp)

add_catch_test(kernel-test kernel_t.cpp)
add_dependencies(kernel-test test_shaders)

