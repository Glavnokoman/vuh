# /test cmake file

if(NOT VUH_BUILD_TESTS)
	return()
endif()

find_package(Catch2 REQUIRED)
function(add_catch_test arg_test_name)
	add_executable(${arg_test_name} ${ARGN})
	target_link_libraries(${arg_test_name} PRIVATE Catch2::Catch)
	target_include_directories( ${arg_test_name} PRIVATE ${PROJECT_SOURCE_DIR}/src )
	add_test(NAME ${arg_test_name} COMMAND ${arg_test_name} )
	# add_dependencies(${arg_test_name} link_shaders_test)
endfunction()

add_catch_test(test_saxpy_sync saxpy_sync_t.cpp array_t.cpp)
target_link_libraries(test_saxpy_sync PRIVATE vuh)
add_dependencies(test_saxpy_sync test_shaders)
