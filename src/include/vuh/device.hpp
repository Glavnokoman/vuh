#pragma once

#include "common.hpp"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace vuh {
class Instance;
struct PhysicalDevice;
class Queue;

///
struct QueueSpec {
	enum Preset {
		  Default=0 ///< selects a minumal required number of queues. One if there is a familiy with compute and transfer capabilities, and two otherwise
		, All=1     ///< selects and all queues from all families supporting compute and(or) transfer.
	};

	QueueSpec(std::uint32_t family_id, std::uint32_t queue_count, std::vector<float> priorities={})
	   : family_id(family_id)
	   , queue_count(queue_count)
	   , priorities(priorities.empty() ? std::vector<float>(queue_count, 0.5f) : std::move(priorities))
	{}

	const std::uint32_t family_id;
	const std::uint32_t queue_count;
	const std::vector<float> priorities;
}; // struct QueueSpecs

/// doc me
class Device: public detail::Resource<VkDevice, vkDestroyDevice> {
public:
	explicit Device(Instance&, const std::vector<const char*>& extensions={});
	Device( Instance&, const PhysicalDevice&
	      , QueueSpec::Preset=QueueSpec::Default, const std::vector<const char*>& extensions={});
	Device( Instance& , const PhysicalDevice&
	      , const std::vector<QueueSpec>& specs, const std::vector<const char*>& extensions={});

	Device( VkDevice device, Instance& instance
	      , const PhysicalDevice& phys_device, const std::vector<QueueSpec>& queue_specs);

	~Device() noexcept;

	Device(const Device&) = delete;
	auto operator= (const Device&)->Device& = delete;
	Device(Device&&) = default; // noexcept;
	auto operator= (Device&&)-> Device& = delete; // noexcept-> Device&;
private: // helpers
private: // data
	Instance& _instance;         ///< non-null
	std::vector<Queue> _queues;  ///< doc me
	std::vector<VkCommandPool> _command_pools; ///< one pool per queue family in use
	Queue* _default_compute;     ///< doc me
	Queue* _default_transfer;    ///< doc me
}; // class Device

} // namespace vuh
