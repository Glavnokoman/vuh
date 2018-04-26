#pragma once

#include <vuh/string_view.h>

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {
	class Instance;

	///
	class Device {
	public:
		explicit Device(vk::PhysicalDevice physdevice);
		explicit Device(vk::PhysicalDevice physdevice, const std::vector<string_view>& layers);
		~Device() noexcept;

		Device(const Device&) = delete;
		Device& operator= (const Device&) = delete;
		Device(Device&&) = default;
		Device& operator= (Device&&) = default;

		auto properties() const-> vk::PhysicalDeviceProperties;
		auto isDiscreteGPU() const-> bool;
		auto computeQueue(uint32_t i = 0)-> vk::Queue;
		auto transferQueue(uint32_t i = 0)-> vk::Queue;
		auto numComputeQueues() const-> uint32_t;
		auto numTransferQueues() const-> uint32_t;
	protected: // data
		vk::Device _dev;
		vk::PhysicalDevice _physdev;
		vk::CommandPool    _cmdpool;
		uint32_t _compute_que_id = uint32_t(-1);   ///< compute queue family id. -1 if device does not have compute-capable queues.
		uint32_t _transfer_ques_id = uint32_t(-1); ///< transfer queue family id, maybe the same as compute queue id.
	}; // class Device
}
