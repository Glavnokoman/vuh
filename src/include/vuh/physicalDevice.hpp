#pragma once

#include <vulkan/vulkan.h>

namespace vuh {
///
struct PhysicalDevice {
	VkPhysicalDevice base;
	auto properties() const-> VkPhysicalDeviceProperties;
}; // struct PhysicalDevice
static_assert(sizeof(PhysicalDevice) == sizeof(VkPhysicalDevice));
static_assert(alignof(PhysicalDevice) == alignof(VkPhysicalDevice));
} // namespace vuh
