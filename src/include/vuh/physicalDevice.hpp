#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace vuh {
/// doc me
struct PhysicalDevice {
	VkPhysicalDevice base;
	operator VkPhysicalDevice() const { return base; }

	auto properties() const-> VkPhysicalDeviceProperties;
	auto queueFamilies() const-> std::vector<VkQueueFamilyProperties>;
}; // struct PhysicalDevice
static_assert(sizeof(PhysicalDevice) == sizeof(VkPhysicalDevice));
static_assert(alignof(PhysicalDevice) == alignof(VkPhysicalDevice));
} // namespace vuh
