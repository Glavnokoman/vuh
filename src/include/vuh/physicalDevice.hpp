#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace vuh {
/// doc me
struct PhysicalDevice {
	VkPhysicalDevice base;
	operator VkPhysicalDevice() const { return base; }

	auto extensions(const char* layer_name) const-> std::vector<VkExtensionProperties>;
	auto extensions() const-> std::vector<VkExtensionProperties>;
	auto layers() const-> std::vector<VkLayerProperties>;
	auto features() const-> VkPhysicalDeviceFeatures;
	auto properties() const-> VkPhysicalDeviceProperties;
	auto queueFamilies() const-> std::vector<VkQueueFamilyProperties>;
}; // struct PhysicalDevice

static_assert(sizeof(PhysicalDevice) == sizeof(VkPhysicalDevice));
static_assert(alignof(PhysicalDevice) == alignof(VkPhysicalDevice));
} // namespace vuh
