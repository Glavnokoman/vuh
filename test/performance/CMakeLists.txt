if(NOT VUH_BUILD_BENCHMARKS)
	return()
endif()

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders
	COMMAND ${CMAKE_COMMAND}
	ARGS -E create_symlink ${PROJECT_SOURCE_DIR}/src/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
	# DEPENDS example_filter
	COMMENT "link shaders to build tests folder"
)
add_custom_target(link_shaders_bench DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders)

find_package(sltbench REQUIRED)

add_executable(bench_saxpy_sync saxpy_sync_b.cpp)
# target_link_libraries(bench_saxpy_sync PRIVATE sltbench saxpy_sync)
add_dependencies(bench_saxpy_sync link_shaders_bench)
