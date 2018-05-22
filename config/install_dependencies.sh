#!/bin/bash
# pip install --user cget
# export CGET_PREFIX=

cget init --std=c++14

git clone --recurse-submodules https://github.com/KhronosGroup/Vulkan-Loader ${CGET_PREFIX}/src/vulkan-loader
cget install ${CGET_PREFIX}/src/vulkan-loader -DBUILD_WSI_WAYLAND_SUPPORT=OFF -DBUILD_TESTS=OFF

git clone --recurse-submodules https://github.com/KhronosGroup/glslang.git ${CGET_PREFIX}/src/glslang
cget install ${CGET_PREFIX}/src/glslang -DENABLE_HLSL=OFF -DENABLE_AMD_EXTENSIONS=OFF -DENABLE_NV_EXTENSIONS=OFF -DENABLE_OPT=OFF

cget install catchorg/Catch2@v2.2.2 -DNO_SELFTEST=ON
