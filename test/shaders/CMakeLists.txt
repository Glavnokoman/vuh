if(VUH_BUILD_TESTS OR VUH_BUILD_BENCHMARKS)
	add_custom_target(test_shaders)

	include(VuhCompileShader)
	vuh_compile_shader(saxpy_shader
	   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/saxpy.comp
	   TARGET ${CMAKE_CURRENT_BINARY_DIR}/saxpy.spv
	)
	add_dependencies(test_shaders saxpy_shader)

	vuh_compile_shader(saxpy_shader_nospec
	   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/saxpy_nospec.comp
	   TARGET ${CMAKE_CURRENT_BINARY_DIR}/saxpy_nospec.spv
	)
	add_dependencies(test_shaders saxpy_shader_nospec)

	vuh_compile_shader(saxpy_shader_nopush
	   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/saxpy_nopush.comp
	   TARGET ${CMAKE_CURRENT_BINARY_DIR}/saxpy_nopush.spv
	)
	add_dependencies(test_shaders saxpy_shader_nopush)

	vuh_compile_shader(saxpy_shader_noth
	   SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/saxpy_noth.comp
	   TARGET ${CMAKE_CURRENT_BINARY_DIR}/saxpy_noth.spv
	)
	add_dependencies(test_shaders saxpy_shader_noth)
endif()
