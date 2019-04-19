#include "physicalDevice.hpp"

#include <algorithm>

namespace vuh {
///
auto PhysicalDevice::properties() const-> VkPhysicalDeviceProperties
{
	auto ret = VkPhysicalDeviceProperties{};
	vkGetPhysicalDeviceProperties(base, &ret);
	return ret;
}

///
std::vector<VkQueueFamilyProperties> PhysicalDevice::queueFamilies() const
{
	auto cnt = std::uint32_t{};
	vkGetPhysicalDeviceQueueFamilyProperties(*this, &cnt, nullptr);
	auto ret = std::vector<VkQueueFamilyProperties>(cnt);
	vkGetPhysicalDeviceQueueFamilyProperties(*this, &cnt, ret.data());
	return ret;
}

} // namespace vuh
