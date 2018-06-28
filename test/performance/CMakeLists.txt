if(NOT VUH_BUILD_BENCHMARKS)
	return()
endif()

find_package(sltbench REQUIRED)

add_executable(bench_saxpy_sync saxpy_b.cpp)
target_link_libraries(bench_saxpy_sync PRIVATE sltbench vuh)
add_dependencies(bench_saxpy_sync test_shaders)
