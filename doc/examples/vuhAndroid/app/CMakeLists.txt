# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.8)

# for shaderc
SET(SHADERC_SRC ${ANDROID_NDK}/sources/third_party/shaderc)
SET(Shaderc_INCLUDE_DIR ${SHADERC_SRC}/libshaderc/include/)
include_directories(${Shaderc_INCLUDE_DIR})

add_library(shaderc_lib STATIC IMPORTED)
set_target_properties(shaderc_lib PROPERTIES IMPORTED_LOCATION
        ${SHADERC_SRC}/libs/${ANDROID_STL}/${ANDROID_ABI}/libshaderc.a)

# for Vulkan
SET(Vulkan_INCLUDE_DIR ${ANDROID_NDK}/sources/third_party/vulkan/src/include/)
SET(Vulkan_LIBRARIES ${ANDROID_NDK}/platforms/${ANDROID_PLATFORM}/arch-${ANDROID_ARCH_NAME}/usr/lib)
if(X86_64)
    SET(Vulkan_LIBRARIES ${Vulkan_LIBRARIES}64)
endif()
SET(Vulkan_LIBRARIES ${Vulkan_LIBRARIES}/libvulkan.so)
add_library(vulkan SHARED IMPORTED)
set_target_properties(vulkan PROPERTIES IMPORTED_LOCATION ${Vulkan_LIBRARIES})

# for vuh
# do'nt define VULKAN_HPP_TYPESAFE_CONVERSION it's not safe on 32bits system
# from vulkan.hpp "32-bit vulkan is not typesafe for handles, so don't allow copy constructors on this platform by default."
add_definitions(-DVK_USE_PLATFORM_ANDROID_KHR=1)
# from https://developer.android.com/sdk/ndk/index.html
#  With the exception of the libraries listed above, native system libraries in the Android platform are not stable and may change in future platform versions.
# Your applications should only make use of the stable native system libraries provided in this NDK.
# Android do'nt enable exception
add_definitions(-DVULKAN_HPP_NO_EXCEPTIONS=1)
SET(VUH_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../)
SET(VUH_BUILD_TESTS OFF)
SET(VUH_BUILD_DOCS OFF)
SET(VUH_BUILD_EXAMPLES OFF)

add_subdirectory(${VUH_ROOT_DIR}src/ ${CMAKE_CURRENT_SOURCE_DIR}/build/)

# for example
SET(Vuh_INCLUDE_PATH ${VUH_ROOT_DIR}src/include)
include_directories(${Vulkan_INCLUDE_DIR})
include_directories(${Vuh_INCLUDE_PATH})

# for make .comp to .spv
SET(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/main/assets)
# from ${ANDROID_NDK}/build/cmake/android.toolchain.cmake
# TOOL_OS_SUFFIX linux or macOS TOOL_OS_SUFFIX "" windows TOOL_OS_SUFFIX ".exe"
SET(GLSLC ${ANDROID_NDK}/shader-tools/${ANDROID_NDK_HOST_SYSTEM_NAME}/glslc${TOOL_OS_SUFFIX})
add_custom_command(OUTPUT GLSL2SPVCMD
        COMMAND ${GLSLC} -c ${SHADER_DIR}/saxpy.comp -o ${SHADER_DIR}/saxpy.spv
        )
add_custom_target(GLSL2SPV
        DEPENDS GLSL2SPVCMD
        )

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        src/main/cpp/glsl2spv.cpp
        src/main/cpp/native-lib.cpp)


add_dependencies(native-lib vuh)
add_dependencies(native-lib GLSL2SPV)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log

        android)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        native-lib

        vulkan

        vuh

        shaderc_lib

        android

        ${log-lib})