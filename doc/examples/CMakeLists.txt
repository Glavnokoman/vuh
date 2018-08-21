if(NOT VUH_BUILD_EXAMPLES)
	return()
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
include(${PROJECT_SOURCE_DIR}/config/VuhCompileShader.cmake)

add_subdirectory(compute_transfer_overlap)
add_subdirectory(mandelbrot)
add_subdirectory(saxpy)
add_subdirectory(spdlog)
