#pragma once

#include "vuh/core/core.hpp"
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
	class Device: public vhn::Device {
	public:
		explicit Device(const vuh::Instance& instance, const vhn::PhysicalDevice& phys_dev);
		~Device() noexcept;

		Device(const Device&);
		auto operator=(Device)-> Device&;
		Device(Device&&) noexcept;
		auto operator=(Device&&) noexcept-> Device&;
		friend auto swap(Device& d1, Device& d2)-> void;

		auto properties() const-> vhn::PhysicalDeviceProperties;
		auto numComputeQueues() const-> uint32_t { return 1u;}
		auto numTransferQueues() const-> uint32_t { return 1u;}
		auto memoryProperties(uint32_t id) const-> vhn::MemoryPropertyFlags;
		auto selectMemory(const vhn::Buffer& buffer, vhn::MemoryPropertyFlags props) const-> uint32_t;
		auto selectMemory(const vhn::Image& im, vhn::MemoryPropertyFlags props) const-> uint32_t;
		auto instance() const-> const vuh::Instance& {return _instance;}
		auto hasSeparateQueues() const-> bool;

		auto computeQueue(uint32_t i = 0)-> vhn::Queue;
		auto transferQueue(uint32_t i = 0)-> vhn::Queue;
		auto alloc(vhn::Buffer buf, uint32_t mem_id, vhn::Result& res)-> vhn::DeviceMemory;
		auto computeCmdPool()-> vhn::CommandPool {return _cmdpool_compute;}
		auto computeCmdBuffer()-> vhn::CommandBuffer& {return _cmdbuf_compute;}
		auto transferCmdPool()-> vhn::CommandPool;
		auto transferCmdBuffer()-> vhn::CommandBuffer&;
		auto createPipeline(vhn::PipelineLayout pipe_layout
		                    , vhn::PipelineCache pipe_cache
		                    , const vhn::PipelineShaderStageCreateInfo& shader_stage_info
							, vhn::Result& res
		                    , vhn::PipelineCreateFlags flags={}
		                    )-> vhn::Pipeline;
		auto instance()-> const vuh::Instance& { return _instance; }
		auto releaseComputeCmdBuffer(vhn::Result& res)-> vhn::CommandBuffer;

		// if fenceFd is support, we can use epoll or select wait for fence complete
		auto supportFenceFd()-> bool;
		
	private: // helpers
		explicit Device(const vuh::Instance& instance, const vhn::PhysicalDevice& phy_dev
		                , const std::vector<vhn::QueueFamilyProperties>& families);
		explicit Device(const vuh::Instance& instance, const vhn::PhysicalDevice& phy_dev
	                   , uint32_t computeFamilyId, uint32_t transferFamilyId);
		auto release() noexcept-> void;

		auto fenceFdSupported() noexcept-> bool;
		auto fenceFdFuncExists() noexcept-> bool;
	private: // data
		const vuh::Instance&  _instance;           ///< refer to Instance object used to create device
		vhn::PhysicalDevice _phy_dev;            ///< handle to associated physical device
		vhn::CommandPool    _cmdpool_compute;    ///< handle to command pool for compute commands
		vhn::CommandBuffer  _cmdbuf_compute;     ///< primary command buffer associated with the compute command pool
		vhn::CommandPool    _cmdpool_transfer;   ///< handle to command pool for transfer instructions. Initialized on first trasnfer request.
		vhn::CommandBuffer  _cmdbuf_transfer;    ///< primary command buffer associated with transfer command pool. Initialized on first transfer request.
		uint32_t _cmp_family_id = uint32_t(-1); ///< compute queue family id. -1 if device does not have compute-capable queues.
		uint32_t _tfr_family_id = uint32_t(-1); ///< transfer queue family id, maybe the same as compute queue id.
		vhn::Result	_res;					///< result of vulkan's api
		bool  _support_fence_fd;			///< if fenceFd is support, we can use epoll or select wait for fence complete
	}; // class Device
}
