add_subdirectory(correctness)
add_subdirectory(performance)
add_subdirectory(shaders)

add_library(test_aux helpers.cpp)
target_include_directories(test_aux PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
