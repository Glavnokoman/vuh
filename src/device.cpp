#include <vuh/device.h>

#include <cassert>
#include <cstdint>
#include <limits>

#ifndef VK_API_VERSION_1_1
#define VK_API_VERSION_1_1 VK_MAKE_VERSION(1, 1, 0)// Patch version should always be set to 0
#endif

namespace {
	/// Create logical device.
	/// Compute and transport queue family id may point to the same queue.
	auto createDevice(const vk::PhysicalDevice& physicalDevice ///< physical device to wrap
	                  , uint32_t compute_family_id             ///< index of queue family supporting compute operations
	                  , uint32_t transfer_family_id            ///< index of queue family supporting transfer operations
	                  , vk::Result& result
	                  )-> vk::Device
	{
		// When creating the device specify what queues it has
		auto p = float(1.0); // queue priority
		auto queueCIs = std::array<vk::DeviceQueueCreateInfo, 2>{};
		queueCIs[0] = vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags()
		                                        , compute_family_id, 1, &p);
		auto n_queues = uint32_t(1);
		if(transfer_family_id != compute_family_id){
			queueCIs[1] = vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags()
			                                        , transfer_family_id, 1, &p);
			n_queues += 1;
		}
		auto devCI = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), n_queues, queueCIs.data());

		auto device = physicalDevice.createDevice(devCI, nullptr);
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		result = device.result;
		VULKAN_HPP_ASSERT(vk::Result::eSuccess == result);
		return device.value;
#else
		result = vk::Result::eSuccess;
		return device;
#endif
	}

	/// @return preffered family id for the desired queue flags combination, or -1 if none is found.
	/// If several queues matching required flags combination is available
	/// selects the one with minimal numeric value of its flags combination.
	auto getFamilyID(const std::vector<vk::QueueFamilyProperties>& queue_families ///< array of queue family properties
	                 , vk::QueueFlags tgtFlag                                     ///< target flags combination
	                 )-> uint32_t
	{
		auto r = uint32_t(-1);
		auto minFlags = std::numeric_limits<VkFlags>::max();

		uint32_t i = 0;
		for(auto&& q : queue_families){ // find the family with a min flag value among all acceptable
			const auto flags_i = q.queueFlags;
			if(0 < q.queueCount
			   && (tgtFlag & flags_i)
			   && VkFlags(flags_i) < minFlags)
			{
				r = i;
				minFlags = VkFlags(flags_i);
			}
			++i;
		}
		return r;
	}

	/// Allocate command buffer
	auto allocCmdBuffer(vk::Device device
	                    , vk::CommandPool pool
	                    , vk::Result& result
	                    , vk::CommandBufferLevel level=vk::CommandBufferLevel::ePrimary
	                    )-> vk::CommandBuffer
	{
		auto commandBufferAI = vk::CommandBufferAllocateInfo(pool, level, 1); // 1 is the command buffer count here
		auto buffers = device.allocateCommandBuffers(commandBufferAI);
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		result = buffers.result;
		VULKAN_HPP_ASSERT(vk::Result::eSuccess == result);
		if(vk::Result::eSuccess == result) {
			return buffers.value[0];
		}
		return vk::CommandBuffer();
#else
		result = vk::Result::eSuccess;
		return buffers[0];
#endif
	}
} // namespace

namespace vuh {
	/// Constructs logical device wrapping the physical device of the given instance.
	Device::Device(const Instance& instance, const vk::PhysicalDevice& phy_dev)
	   : Device(instance, phy_dev, phy_dev.getQueueFamilyProperties())
	{}

	/// Helper constructor.
	Device::Device(const Instance& instance, const vk::PhysicalDevice& phy_dev
	              , const std::vector<vk::QueueFamilyProperties>& familyProperties
	              )
	   : Device(instance, phy_dev, getFamilyID(familyProperties, vk::QueueFlagBits::eCompute)
	            , getFamilyID(familyProperties, vk::QueueFlagBits::eTransfer))
	{}

	/// Helper constructor
	Device::Device(const Instance& instance, const vk::PhysicalDevice& phy_dev
	               , uint32_t computeFamilyId, uint32_t transferFamilyId
	               )
	  : vk::Device(createDevice(phy_dev, computeFamilyId, transferFamilyId, _result))
	  , _instance(instance)
	  , _phy_dev(phy_dev)
	  , _cmp_family_id(computeFamilyId)
	  , _tfr_family_id(transferFamilyId)
	  , _support_fence_fd(fenceFdSupported())
	{
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		auto pool = createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer
													 , computeFamilyId});
		_result = pool.result;
		VULKAN_HPP_ASSERT(vk::Result::eSuccess == _result);
		if(vk::Result::eSuccess == _result) {
			_cmdpool_compute = pool.value;
			_cmdbuf_compute = allocCmdBuffer(*this, _cmdpool_compute, _result);
			VULKAN_HPP_ASSERT(vk::Result::eSuccess == _result);
			if(vk::Result::eSuccess == _result) {
				if (_tfr_family_id == _cmp_family_id) {
					_cmdpool_transfer = _cmdpool_compute;
					_cmdbuf_transfer = _cmdbuf_compute;
				} else {
					auto transfer = createCommandPool(
							{vk::CommandPoolCreateFlagBits::eResetCommandBuffer, _tfr_family_id});
					_result = transfer.result;
					VULKAN_HPP_ASSERT(vk::Result::eSuccess == _result);
					if (vk::Result::eSuccess == _result) {
						_cmdpool_transfer = transfer.value;
						_cmdbuf_transfer = allocCmdBuffer(*this, _cmdpool_transfer, _result);
					}
				}
			}
		}
		VULKAN_HPP_ASSERT(vk::Result::eSuccess == _result);
		if(vk::Result::eSuccess != _result) {
			release(); // because vk::Device does not know how to clean after itself
		}
#else
		try {
			_cmdpool_compute = createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer
			                                     , computeFamilyId});
			_cmdbuf_compute = allocCmdBuffer(*this, _cmdpool_compute, _result);
			if(_tfr_family_id == _cmp_family_id){
				_cmdpool_transfer = _cmdpool_compute;
				_cmdbuf_transfer = _cmdbuf_compute;
			} else {
				_cmdpool_transfer = createCommandPool(
				                 {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, _tfr_family_id});
				_cmdbuf_transfer = allocCmdBuffer(*this, _cmdpool_transfer, _result);
			}
		} catch(vk::Error&) {
			release(); // because vk::Device does not know how to clean after itself
			throw;
		}
#endif
	}

	/// release resources associated with device
	auto Device::release() noexcept-> void {
		if(static_cast<vk::Device&>(*this)){
			if(_tfr_family_id != _cmp_family_id){
				if(bool(_cmdpool_transfer) && bool(_cmdbuf_transfer)) {
					freeCommandBuffers(_cmdpool_transfer, 1, &_cmdbuf_transfer);
				}
				if(bool(_cmdpool_transfer)) {
					destroyCommandPool(_cmdpool_transfer);
				}
			}
			if(bool(_cmdpool_compute) && bool(_cmdbuf_compute)) {
				freeCommandBuffers(_cmdpool_compute, 1, &_cmdbuf_compute);
			}
			if(bool(_cmdpool_compute)) {
				destroyCommandPool(_cmdpool_compute);
			}

			vk::Device::destroy();
		}
	}

	// if fenceFd is support, we can use epoll or select wait for fence complete
	// following https://www.khronos.org/registry/vulkan/specs/1.0-extensions/html/vkspec.html#VK_KHR_external_fence
	// vulkan 1.1 support VK_KHR_external_fence default (Promoted to Vulkan 1.1)
	// vulkan 1.0 ,need VK_KHR_external_fence extension on Android 1.0.54 import this extension
	// following https://android.googlesource.com/platform/frameworks%2Fnative/+/9492f99cb57d97aa5df908773738fe7fe6a86acf
	auto Device::fenceFdSupported() noexcept-> bool {
		auto props = properties();
		if (props.apiVersion >= VK_API_VERSION_1_1) {
			return true;
		} else {
#ifdef VULKAN_HPP_NO_EXCEPTIONS
			const auto em_extensions = _phy_dev.enumerateDeviceExtensionProperties();
			auto avail_extensions = em_extensions.value;
			VULKAN_HPP_ASSERT(vk::Result::eSuccess == em_extensions.result);
			if(vk::Result::eSuccess != em_extensions.result) {
				avail_extensions.clear();
			}
#else
			const auto avail_extensions = _phy_dev.enumerateDeviceExtensionProperties();
#endif
			for(int i = 0; i< avail_extensions.size(); i++) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
				if(0 == strcmp(avail_extensions[i].extensionName,VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME)) {
#else
				if(0 == strcmp(avail_extensions[i].extensionName,VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME)) {
#endif
					return true;
				}
			}
		}

		return false;
	}

	/// Release resources associated with a logical device
	Device::~Device() noexcept {
		release();
	}

	/// Copy constructor. Creates new handle to the same physical device, and recreates associated pools
	Device::Device(const Device& other)
	   : Device(other._instance, other._phy_dev, other._cmp_family_id, other._tfr_family_id)
	{}

	/// Copy assignment. Created new handle to the same physical device and recreates associated pools.
	auto Device::operator=(Device other)-> Device& {
		swap(*this, other);
		return *this;
	}

	/// Move constructor.
	Device::Device(Device&& other) noexcept
	   : vk::Device(std::move(other))
	   , _instance(other._instance)
	   , _phy_dev(other._phy_dev)
	   , _cmdpool_compute(other._cmdpool_compute)
	   , _cmdbuf_compute(other._cmdbuf_compute)
	   , _cmdpool_transfer(other._cmdpool_transfer)
	   , _cmdbuf_transfer(other._cmdbuf_transfer)
	   , _cmp_family_id(other._cmp_family_id)
	   , _tfr_family_id(other._tfr_family_id)
	   , _support_fence_fd(other._support_fence_fd)
	{
		static_cast<vk::Device&>(other)= nullptr;
	}

	/// Move assignment.
	auto Device::operator=(Device&& o) noexcept-> Device& {
		swap(*this, o);
		return *this;
	}

	/// Swap the guts of two devices (member-wise)
	auto swap(Device& d1, Device& d2)-> void {
		using std::swap;
		swap((vk::Device&)d1     , (vk::Device&)d2     );
		swap(d1._phy_dev         , d2._phy_dev         );
		swap(d1._cmdpool_compute , d2._cmdpool_compute );
		swap(d1._cmdbuf_compute  , d2._cmdbuf_compute  );
		swap(d1._cmdpool_transfer, d2._cmdpool_transfer);
		swap(d1._cmdbuf_transfer , d2._cmdbuf_transfer );
		swap(d1._cmp_family_id   , d2._cmp_family_id   );
		swap(d1._tfr_family_id   , d2._tfr_family_id   );
		swap(d1._support_fence_fd, d2._support_fence_fd);
	}

	/// @return physical device properties
	auto Device::properties() const-> vk::PhysicalDeviceProperties {
		return _phy_dev.getProperties();
	}

	/// @return memory properties of the memory with given id
	auto Device::memoryProperties(uint32_t id) const-> vk::MemoryPropertyFlags {
		return _phy_dev.getMemoryProperties().memoryTypes[id].propertyFlags;
	}

	/// Find first memory matching desired properties.
	/// Does NOT check for free space availability, only matches the properties.
	/// @return id of the suitable memory, -1 if no suitable memory found.
	auto Device::selectMemory(const vk::Buffer& buffer, vk::MemoryPropertyFlags properties
	                          ) const-> uint32_t
	{
		auto memProperties = _phy_dev.getMemoryProperties();
		auto memoryReqs = getBufferMemoryRequirements(buffer);
		for(uint32_t i = 0; i < memProperties.memoryTypeCount; ++i){
			if( (memoryReqs.memoryTypeBits & (1u << i))
			    && ((properties & memProperties.memoryTypes[i].propertyFlags) == properties))
			{
				return i;
			}
		}
		return uint32_t(-1);
	}

	/// Find first memory matching desired properties.
	/// Does NOT check for free space availability, only matches the properties.
	/// @return id of the suitable memory, -1 if no suitable memory found.
	auto Device::selectMemory(const vk::Image& image, vk::MemoryPropertyFlags properties
	) const-> uint32_t
	{
		auto memProperties = _phy_dev.getMemoryProperties();
		auto memoryReqs = getImageMemoryRequirements(image);
		for(uint32_t i = 0; i < memProperties.memoryTypeCount; ++i){
			if( (memoryReqs.memoryTypeBits & (1u << i))
				&& ((properties & memProperties.memoryTypes[i].propertyFlags) == properties))
			{
				return i;
			}
		}
		return uint32_t(-1);
	}

	/// @return true if compute queues family is different from that for transfer queues
	auto Device::hasSeparateQueues() const-> bool {
		return _cmp_family_id == _tfr_family_id;
	}

	/// @return id of the queue family supporting compute operations
	auto Device::computeQueue(uint32_t i)-> vk::Queue {
		return getQueue(_cmp_family_id, i);
	}

	/// Create compute pipeline with a given layout.
	/// Shader stage info incapsulates the shader and layout&values of specialization constants.
	auto Device::createPipeline(vk::PipelineLayout pipe_layout
	                            , vk::PipelineCache pipe_cache
	                            , const vk::PipelineShaderStageCreateInfo& shader_stage_info
	                            , vk::Result& result
	                            , vk::PipelineCreateFlags flags
	                            )-> vk::Pipeline
	{
		auto pipelineCI = vk::ComputePipelineCreateInfo(flags
																		, shader_stage_info, pipe_layout);
		auto pipeline = createComputePipeline(pipe_cache, pipelineCI, nullptr);
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		result = pipeline.result;
		VULKAN_HPP_ASSERT(vk::Result::eSuccess == result);
		return pipeline.value;
#else
		result = vk::Result::eSuccess;
		return pipeline;
#endif
	}

	/// Detach the current compute command buffer for sync operations and create the new one.
	/// @return the old buffer handle
	auto Device::releaseComputeCmdBuffer(vk::Result& result)-> vk::CommandBuffer {
		auto new_buffer = allocCmdBuffer(*this, _cmdpool_compute, result);
		if (vk::Result::eSuccess == result) {
			std::swap(new_buffer, _cmdbuf_compute);
			if (_tfr_family_id == _cmp_family_id) {
				_cmdbuf_transfer = _cmdbuf_compute;
			}
		}
		return new_buffer;
	}

	auto Device::supportFenceFd()-> bool {
		return _support_fence_fd;
	}

	/// @return i-th queue in the family supporting transfer commands.
	auto Device::transferQueue(uint32_t i)-> vk::Queue {
		return getQueue(_tfr_family_id, i);
	}

	/// Allocate device memory for the buffer in the memory with given id.
	auto Device::alloc(vk::Buffer buf, uint32_t memory_id, vk::Result& result)-> vk::DeviceMemory {
	   auto memoryReqs = getBufferMemoryRequirements(buf);
	   auto allocInfo = vk::MemoryAllocateInfo(memoryReqs.size, memory_id);
	   auto allocMem = allocateMemory(allocInfo);
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		result = allocMem.result;
		VULKAN_HPP_ASSERT(vk::Result::eSuccess == result);
		return allocMem.value;
#else
		result = vk::Result::eSuccess;
		return allocMem;
#endif
	}

	/// @return handle to command pool for transfer command buffers
	auto Device::transferCmdPool()-> vk::CommandPool { return _cmdpool_transfer; }

	/// @return handle to command buffer for syncronous transfer commands
	auto Device::transferCmdBuffer()-> vk::CommandBuffer& { return _cmdbuf_transfer; }
} // namespace vuh
