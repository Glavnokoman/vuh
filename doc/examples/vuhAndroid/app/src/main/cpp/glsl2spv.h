//
// Created by longsky on 2019/2/14.
//

#ifndef VUHANDROID_GLSL2SPV_H
#define VUHANDROID_GLSL2SPV_H
#include "vulkan/vulkan.hpp"

bool glsl2spv(const VkShaderStageFlagBits shader_type, const char *pshader, std::vector<unsigned int> &spirv);

#endif //VUHANDROID_GLSL2SPV_H
