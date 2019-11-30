#pragma once

#include <vulkan/vulkan.hpp>

// VULKAN_HPP_NAMESPACE
#if !defined(vhn)
    #if defined(VULKAN_HPP_NAMESPACE)
        #define vhn VULKAN_HPP_NAMESPACE
    #else
        #define vhn vk
    #endif
#endif


