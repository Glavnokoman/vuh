#include "physicalDevice.hpp"

namespace vuh {
///
auto PhysicalDevice::properties() const-> VkPhysicalDeviceProperties
{
	auto ret = VkPhysicalDeviceProperties{};
	vkGetPhysicalDeviceProperties(base, &ret);
	return ret;
}

} // namespace vuh
