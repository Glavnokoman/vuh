
find_program(GlslangValidator NAMES glslangValidator DOC "glsl to SPIR-V compiler")
if(NOT GlslangValidator)
   message(FATAL_ERROR "failed to find glslangValidator")
endif()

function(compile_shader)
   set(OneValueArgs SOURCE TARGET)
   cmake_parse_arguments(COMPILE_SHADER "" "${OneValueArgs}" "" ${ARGN})

   get_filename_component(TargetDir ${COMPILE_SHADER_TARGET} DIRECTORY)
   add_custom_command(
      COMMAND ${CMAKE_COMMAND} ARGS -E make_directory ${TargetDir}
      COMMAND ${GlslangValidator} ARGS -V ${COMPILE_SHADER_SOURCE} -o ${COMPILE_SHADER_TARGET}
      DEPENDS ${COMPILE_SHADER_SOURCE}
      OUTPUT ${COMPILE_SHADER_TARGET}
   )
   add_custom_target(${ARGV0} DEPENDS ${COMPILE_SHADER_TARGET})
endfunction()
