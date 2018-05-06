if(VUH_BUILD_TESTS OR VUH_BUILD_BENCHMARKS)
   include(CompileShader)

   compile_shader(saxpy_shader
      SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/saxpy.comp
      TARGET ${CMAKE_CURRENT_BINARY_DIR}/saxpy.spv
   )
endif()
