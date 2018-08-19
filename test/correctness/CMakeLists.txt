# /test cmake file

if(NOT VUH_BUILD_TESTS)
	return()
endif()

find_package(Catch2 REQUIRED)
function(add_catch_test arg_test_name)
	add_executable(${arg_test_name} ${ARGN})
	target_link_libraries(${arg_test_name} PRIVATE Catch2::Catch2)
	target_include_directories( ${arg_test_name} PRIVATE ${PROJECT_SOURCE_DIR}/src )
	add_test(NAME ${arg_test_name} COMMAND ${arg_test_name} )
	# add_dependencies(${arg_test_name} link_shaders_test)
endfunction()

add_catch_test(test_vuh
	array_async_t.cpp
	array_t.cpp
	saxpy_async_t.cpp
	saxpy_sync_t.cpp
)
target_link_libraries(test_vuh PRIVATE vuh)
add_dependencies(test_vuh test_shaders)
