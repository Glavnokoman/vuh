#!/bin/bash
# pip install --user cget
# export CGET_PREFIX=

cget init --std=c++14

# cget install KhronosGroup/Vulkan-Headers

# https://github.com/Glavnokoman/Vulkan-Loader.git
git clone --branch sdk-1.1.77.0 --depth 1 --recurse-submodules https://github.com/KhronosGroup/Vulkan-Headers.git ${CGET_PREFIX}/src/vulkan-headers
cget install ${CGET_PREFIX}/src/vulkan-headers

#git clone --recurse-submodules -b fix_find_vulkan_headers https://github.com/Glavnokoman/Vulkan-Loader.git ${CGET_PREFIX}/src/vulkan-loader
git clone --branch sdk-1.1.77.0 --depth 1 --recurse-submodules https://github.com/KhronosGroup/Vulkan-Loader ${CGET_PREFIX}/src/vulkan-loader
cget install ${CGET_PREFIX}/src/vulkan-loader -DBUILD_WSI_WAYLAND_SUPPORT=OFF -DBUILD_TESTS=OFF

git clone --branch 6.2.2596 --depth 1 --recurse-submodules https://github.com/KhronosGroup/glslang.git ${CGET_PREFIX}/src/glslang
cget install ${CGET_PREFIX}/src/glslang -DENABLE_HLSL=OFF -DENABLE_AMD_EXTENSIONS=OFF -DENABLE_NV_EXTENSIONS=OFF -DENABLE_OPT=OFF

cget install catchorg/Catch2@v2.2.2 -DNO_SELFTEST=ON
