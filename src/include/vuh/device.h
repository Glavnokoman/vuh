#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {
	class Instance;

	/// Logical device packed with associated command pools and buffers.
	/// Holds the pool(s) for transfer and compute operations as well as command
	/// buffers for sync operations.
	/// When the copy of the Device object is made it recreates all underlying
	/// structures access to which needs to be synchrnized, while still referring
	/// to the same physical device. Such that copying the Device object might be
	/// a convenient (although somewhat resource consuming) way to use device from
	/// different threads.
	class Device: public vk::Device {
	public:
		explicit Device(vuh::Instance& instance, vk::PhysicalDevice physdevice);
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
		auto instance() const-> const vuh::Instance& {return _instance;}
		auto hasSeparateQueues() const-> bool;

		auto computeQueue(uint32_t i = 0)-> vk::Queue;
		auto transferQueue(uint32_t i = 0)-> vk::Queue;
		auto alloc(vk::Buffer buf, uint32_t memory_id)-> vk::DeviceMemory;
		auto computeCmdPool()-> vk::CommandPool {return _cmdpool_compute;}
		auto computeCmdBuffer()-> vk::CommandBuffer& {return _cmdbuf_compute;}
		auto transferCmdPool()-> vk::CommandPool;
		auto transferCmdBuffer()-> vk::CommandBuffer&;
		auto createPipeline(vk::PipelineLayout pipe_layout
		                    , vk::PipelineCache pipe_cache
		                    , const vk::PipelineShaderStageCreateInfo& shader_stage_info
		                    , vk::PipelineCreateFlags flags={}
		                    )-> vk::Pipeline;
		auto instance()-> vuh::Instance& { return _instance; }
		auto releaseComputeCmdBuffer()-> vk::CommandBuffer;
		
	private: // helpers
		explicit Device(vuh::Instance& instance, vk::PhysicalDevice physdevice
		                , const std::vector<vk::QueueFamilyProperties>& families);
		explicit Device(vuh::Instance& instance, vk::PhysicalDevice physdevice
	                   , uint32_t computeFamilyId, uint32_t transferFamilyId);
		auto release() noexcept-> void;
	private: // data
		vuh::Instance&     _instance;           ///< refer to Instance object used to create device
		vk::PhysicalDevice _physdev;            ///< handle to associated physical device
		vk::CommandPool    _cmdpool_compute;    ///< handle to command pool for compute commands
		vk::CommandBuffer  _cmdbuf_compute;     ///< primary command buffer associated with the compute command pool
		vk::CommandPool    _cmdpool_transfer;   ///< handle to command pool for transfer instructions. Initialized on first trasnfer request.
		vk::CommandBuffer  _cmdbuf_transfer;    ///< primary command buffer associated with transfer command pool. Initialized on first transfer request.
		uint32_t _cmp_family_id = uint32_t(-1); ///< compute queue family id. -1 if device does not have compute-capable queues.
		uint32_t _tfr_family_id = uint32_t(-1); ///< transfer queue family id, maybe the same as compute queue id.
	}; // class Device
}
