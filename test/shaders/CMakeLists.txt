if(VUH_BUILD_TESTS OR VUH_BUILD_BENCHMARKS)
   add_custom_target(test_shaders)

   include(CompileShader)
   compile_shader(saxpy_shader
      SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/saxpy.comp
      TARGET ${CMAKE_CURRENT_BINARY_DIR}/saxpy.spv
   )
   add_dependencies(test_shaders saxpy_shader)

   compile_shader(saxpy_shader_nospec
      SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/saxpy_nospec.comp
      TARGET ${CMAKE_CURRENT_BINARY_DIR}/saxpy_nospec.spv
   )
   add_dependencies(test_shaders saxpy_shader_nospec)

   compile_shader(saxpy_shader_nopush
      SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/saxpy_nopush.comp
      TARGET ${CMAKE_CURRENT_BINARY_DIR}/saxpy_nopush.spv
   )
   add_dependencies(test_shaders saxpy_shader_nopush)
endif()
