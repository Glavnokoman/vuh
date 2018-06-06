include(CMakeFindDependencyMacro)
find_dependency(Vulkan)
include("${CMAKE_CURRENT_LIST_DIR}/VuhTargets.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/VuhCompileShader.cmake")
