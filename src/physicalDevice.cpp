#include "physicalDevice.hpp"
#include "error.hpp"

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

///
auto PhysicalDevice::extensions(const char* layer_name) const-> std::vector<VkExtensionProperties> {
	auto ret = std::vector<VkExtensionProperties>{};
	auto cnt = std::uint32_t{};
	VUH_CHECK_RET(vkEnumerateDeviceExtensionProperties(*this, layer_name, &cnt, nullptr), ret);
	ret.resize(cnt);
	VUH_CHECK_RET(vkEnumerateDeviceExtensionProperties(*this, layer_name, &cnt, ret.data()), ret);
	return ret;
}

/// @return extensions for all available layers
auto PhysicalDevice::extensions() const-> std::vector<VkExtensionProperties> {
	auto ret = extensions(nullptr);
	VUH_CHECKOUT_RET(ret);
	for(const auto& l: layers()){
		auto lext = extensions(l.layerName);
		VUH_CHECKOUT_RET(ret);
		ret.insert(ret.end(), std::begin(lext), std::end(lext));
	}
	return ret;
}

///
auto PhysicalDevice::layers() const-> std::vector<VkLayerProperties> {
	auto ret = std::vector<VkLayerProperties>{};
	auto cnt = std::uint32_t{};
	VUH_CHECK_RET(vkEnumerateDeviceLayerProperties(*this, &cnt, nullptr), ret);
	ret.resize(cnt);
	VUH_CHECK_RET(vkEnumerateDeviceLayerProperties(*this, &cnt, ret.data()), ret);
	return ret;
}

///
auto PhysicalDevice::features() const-> VkPhysicalDeviceFeatures {
	auto ret = VkPhysicalDeviceFeatures{};
	vkGetPhysicalDeviceFeatures(*this, &ret);
	return ret;
}

} // namespace vuh
