#!/bin/bash
# pip install --user cget
# export CGET_PREFIX=

cget init --std=c++14

# cget install KhronosGroup/Vulkan-Headers

git clone --branch sdk-1.1.82.0 --depth 1 --recurse-submodules https://github.com/KhronosGroup/Vulkan-Headers.git ${CGET_PREFIX}/src/vulkan-headers
cget install ${CGET_PREFIX}/src/vulkan-headers

git clone --branch sdk-1.1.82.0 --depth 1 --recurse-submodules https://github.com/KhronosGroup/Vulkan-Loader ${CGET_PREFIX}/src/vulkan-loader
cget install ${CGET_PREFIX}/src/vulkan-loader -DBUILD_WSI_WAYLAND_SUPPORT=OFF -DBUILD_TESTS=OFF

git clone --branch 7.8.2853 --depth 1 --recurse-submodules https://github.com/KhronosGroup/glslang.git ${CGET_PREFIX}/src/glslang
cget install ${CGET_PREFIX}/src/glslang -DENABLE_HLSL=OFF -DENABLE_AMD_EXTENSIONS=OFF -DENABLE_NV_EXTENSIONS=OFF -DENABLE_OPT=OFF

cget install catchorg/Catch2@v2.3.0 -DBUILD_TESTING=OFF
