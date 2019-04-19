#pragma once

#include "common.hpp"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace vuh {
class Instance;
struct PhysicalDevice;
///
struct QueueSpec {
	enum Preset {Default=0};

	std::uint32_t family_id;
	std::uint32_t queue_count;
	std::vector<float> priorities={};
}; // struct QueueSpecs

/// doc me
class Device: public detail::Resource<VkDevice, vkDestroyDevice> {
public:
	static auto wrap(VkDevice device, Instance& instance);
	explicit Device(Instance&, const PhysicalDevice&, QueueSpec::Preset=QueueSpec::Default);
	Device(Instance&, const PhysicalDevice&, const std::vector<QueueSpec>& specs);
private: // data
}; // class Device

} // namespace vuh
