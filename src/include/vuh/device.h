#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {
	class Instance;

	///
	class Device {
	public:
		explicit Device(vk::PhysicalDevice physdevice, std::vector<const char*> layers={});
		~Device() noexcept;

		Device(const Device&);
		auto operator=(Device)-> Device&;
		Device(Device&&) noexcept;
		auto operator=(Device&&) noexcept-> Device&;
		friend auto swap(Device& d1, Device& d2)-> void;

		auto properties() const-> vk::PhysicalDeviceProperties;
		auto numComputeQueues() const-> uint32_t { return 1u;}
		auto numTransferQueues() const-> uint32_t { return 1u;}
		auto memoryProperties(uint32_t id) const-> vk::MemoryPropertyFlags;
		auto selectMemory(vk::Buffer buffer, vk::MemoryPropertyFlags properties) const-> uint32_t;

		auto computeQueue(uint32_t i = 0)-> vk::Queue;
		auto transferQueue(uint32_t i = 0)-> vk::Queue;
		auto makeBuffer(uint32_t size, vk::BufferUsageFlags usage)-> vk::Buffer;
		auto alloc(vk::Buffer buf, uint32_t memory_id)-> vk::DeviceMemory;
		auto bindBufferMemory(vk::Buffer, vk::DeviceMemory, uint32_t offset=0)-> void;
		auto freeMemory(vk::DeviceMemory)-> void;
		auto mapMemory(vk::DeviceMemory, uint32_t size, uint32_t offset=0)-> void*;
		auto destroyBuffer(vk::Buffer)-> void;
		auto computeCmdBuffer()-> vk::CommandBuffer {return _cmdbuf_compute;}
		auto transferCmdBuffer()-> vk::CommandBuffer;
		auto allocComputeCommandBuffer(vk::CommandBufferLevel level=vk::CommandBufferLevel::ePrimary
		                              )-> vk::CommandBuffer;

	private: // helpers
		explicit Device(vk::PhysicalDevice physdevice, std::vector<const char*> layers
		                , const std::vector<vk::QueueFamilyProperties>& families);
		explicit Device(vk::PhysicalDevice physdevice, std::vector<const char*> layers
	                   , uint32_t computeFamilyId, uint32_t transferFamilyId);
		auto release() noexcept-> void;
	protected: // data
		vk::Device _dev;                           ///< logical device handle
		vk::PhysicalDevice _physdev;               ///< handle to associated physical device
		vk::CommandPool    _cmdpool_compute;       ///< handle to command pool for compute commands
		vk::CommandBuffer  _cmdbuf_compute;        ///< primary command buffer associated with the compute command pool
		vk::CommandPool    _cmdpool_transfer;      ///< handle to command pool for transfer instructions. Initialized on first trasnfer request.
		vk::CommandBuffer  _cmdbuf_transfer;       ///< primary command buffer associated with transfer command pool. Initialized on first transfer request.
		std::vector<const char*> _layers;          ///< layers activated on device. Better be the same as on underlying instance.
		uint32_t _computeFamilyId = uint32_t(-1);  ///< compute queue family id. -1 if device does not have compute-capable queues.
		uint32_t _transferFamilyId = uint32_t(-1); ///< transfer queue family id, maybe the same as compute queue id.
	}; // class Device
}
