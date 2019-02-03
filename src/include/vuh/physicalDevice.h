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
private: // data
}; // class PhysicalDevice

} // namespace vuh
