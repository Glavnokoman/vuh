#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace vuh {

auto available_layers()-> std::vector<VkLayerProperties>;
auto available_extensions()-> std::vector<VkExtensionProperties>;
auto available_extensions(const char* layerName)-> std::vector<VkExtensionProperties>;

auto selectMemory(VkPhysicalDevice device
                 , VkMemoryRequirements requirements, VkMemoryPropertyFlags properties
                 )-> std::vector<std::uint32_t>;

} // namespace vuh
