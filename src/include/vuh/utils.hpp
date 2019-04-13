#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace vuh {

auto available_layers()-> std::vector<VkLayerProperties>;
auto available_extensions()-> std::vector<VkExtensionProperties>;
auto available_extensions(const char* layerName)-> std::vector<VkExtensionProperties>;

} // namespace vuh
