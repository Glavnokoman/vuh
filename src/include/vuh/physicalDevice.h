#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {

class Device;
class QueueFamily;

/// doc me
class PhysicalDevice: public vk::PhysicalDevice {
public:
	explicit PhysicalDevice();

	auto queueFamilies() const-> std::vector<QueueFamily>;
	template<class... Ts> auto computeDevice(Ts&&... opts)-> vuh::Device;

	auto streamCount() const-> std::uint32_t; ///< return max number of "streams" the device can support
private: // data
}; // class PhysicalDevice

} // namespace vuh
