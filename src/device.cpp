#include <vuh/device.h>

#include <cassert>
#include <cstdint>
#include <limits>

namespace {
	/// Create logical device.
	/// Compute and transport queue family id may point to the same queue.
	auto createDevice(const vk::PhysicalDevice& physicalDevice ///< physical device to wrap
	                  , uint32_t compute_family_id             ///< index of queue family supporting compute operations
	                  , uint32_t transfer_family_id            ///< index of queue family supporting transfer operations
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

		return physicalDevice.createDevice(devCI, nullptr);
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
	                    , vk::CommandBufferLevel level=vk::CommandBufferLevel::ePrimary
	                    )-> vk::CommandBuffer
	{
		auto commandBufferAI = vk::CommandBufferAllocateInfo(pool, level, 1); // 1 is the command buffer count here
		return device.allocateCommandBuffers(commandBufferAI)[0];
	}
} // namespace

namespace vuh {
	/// Constructs logical device wrapping the physical device of the given instance.
	Device::Device(Instance& instance, vk::PhysicalDevice physical_device)
	   : Device(instance, physical_device, physical_device.getQueueFamilyProperties())
	{}

	/// Helper constructor.
	Device::Device(Instance& instance, vk::PhysicalDevice physdevice
	              , const std::vector<vk::QueueFamilyProperties>& familyProperties
	              )
	   : Device(instance, physdevice, getFamilyID(familyProperties, vk::QueueFlagBits::eCompute)
	            , getFamilyID(familyProperties, vk::QueueFlagBits::eTransfer))
	{}

	/// Helper constructor
	Device::Device(Instance& instance, vk::PhysicalDevice physdevice
	               , uint32_t computeFamilyId, uint32_t transferFamilyId
	               )
	  : vk::Device(createDevice(physdevice, computeFamilyId, transferFamilyId))
	  , _instance(instance)
	  , _physdev(physdevice)
	  , _cmp_family_id(computeFamilyId)
	  , _tfr_family_id(transferFamilyId)
	{
		try {
			_cmdpool_compute = createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer
			                                     , computeFamilyId});
			_cmdbuf_compute = allocCmdBuffer(*this, _cmdpool_compute);
			if(_tfr_family_id == _cmp_family_id){
				_cmdpool_transfer = _cmdpool_compute;
				_cmdbuf_transfer = _cmdbuf_compute;
			} else {
				_cmdpool_transfer = createCommandPool(
				                 {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, _tfr_family_id});
				_cmdbuf_transfer = allocCmdBuffer(*this, _cmdpool_transfer);
			}
		} catch(vk::Error&) {
			release(); // because vk::Device does not know how to clean after itself
			throw;
		}
	}

	/// release resources associated with device
	auto Device::release() noexcept-> void {
		if(static_cast<vk::Device&>(*this)){
			if(_tfr_family_id != _cmp_family_id){
				freeCommandBuffers(_cmdpool_transfer, 1, &_cmdbuf_transfer);
				destroyCommandPool(_cmdpool_transfer);
			}
			freeCommandBuffers(_cmdpool_compute, 1, &_cmdbuf_compute);
			destroyCommandPool(_cmdpool_compute);

			vk::Device::destroy();
		}
	}

	/// Release resources associated with a logical device
	Device::~Device() noexcept {
		release();
	}

	/// Copy constructor. Creates new handle to the same physical device, and recreates associated pools
	Device::Device(const Device& other)
	   : Device(other._instance, other._physdev, other._cmp_family_id, other._tfr_family_id)
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
	   , _physdev(other._physdev)
	   , _cmdpool_compute(other._cmdpool_compute)
	   , _cmdbuf_compute(other._cmdbuf_compute)
	   , _cmdpool_transfer(other._cmdpool_transfer)
	   , _cmdbuf_transfer(other._cmdbuf_transfer)
	   , _cmp_family_id(other._cmp_family_id)
	   , _tfr_family_id(other._tfr_family_id)
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
		swap(d1._physdev         , d2._physdev         );
		swap(d1._cmdpool_compute , d2._cmdpool_compute );
		swap(d1._cmdbuf_compute  , d2._cmdbuf_compute  );
		swap(d1._cmdpool_transfer, d2._cmdpool_transfer);
		swap(d1._cmdbuf_transfer , d2._cmdbuf_transfer );
		swap(d1._cmp_family_id   , d2._cmp_family_id   );
		swap(d1._tfr_family_id   , d2._tfr_family_id   );
	}

	/// @return physical device properties
	auto Device::properties() const-> vk::PhysicalDeviceProperties {
		return _physdev.getProperties();
	}

	/// @return memory properties of the memory with given id
	auto Device::memoryProperties(uint32_t id) const-> vk::MemoryPropertyFlags {
		return _physdev.getMemoryProperties().memoryTypes[id].propertyFlags;
	}

	/// Find first memory matching desired properties.
	/// Does NOT check for free space availability, only matches the properties.
	/// @return id of the suitable memory, -1 if no suitable memory found.
	auto Device::selectMemory(vk::Buffer buffer, vk::MemoryPropertyFlags properties
	                          ) const-> uint32_t
	{
		auto memProperties = _physdev.getMemoryProperties();
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
	                            , vk::PipelineCreateFlags flags
	                            )-> vk::Pipeline
	{
		auto pipelineCI = vk::ComputePipelineCreateInfo(flags
																		, shader_stage_info, pipe_layout);
		return createComputePipeline(pipe_cache, pipelineCI, nullptr);
		
	}

	/// Detach the current compute command buffer for sync operations and create the new one.
	/// @return the old buffer handle
	auto Device::releaseComputeCmdBuffer()-> vk::CommandBuffer {
		auto new_buffer = allocCmdBuffer(*this, _cmdpool_compute);
		std::swap(new_buffer, _cmdbuf_compute);
		if(_tfr_family_id == _cmp_family_id){
			_cmdbuf_transfer = _cmdbuf_compute;
		}
		return new_buffer;
	}

	/// @return i-th queue in the family supporting transfer commands.
	auto Device::transferQueue(uint32_t i)-> vk::Queue {
		return getQueue(_tfr_family_id, i);
	}

	/// Allocate device memory for the buffer in the memory with given id.
	auto Device::alloc(vk::Buffer buf, uint32_t memory_id)-> vk::DeviceMemory {
	   auto memoryReqs = getBufferMemoryRequirements(buf);
	   auto allocInfo = vk::MemoryAllocateInfo(memoryReqs.size, memory_id);
		return allocateMemory(allocInfo);
	}

	/// @return handle to command pool for transfer command buffers
	auto Device::transferCmdPool()-> vk::CommandPool { return _cmdpool_transfer; }

	/// @return handle to command buffer for syncronous transfer commands
	auto Device::transferCmdBuffer()-> vk::CommandBuffer& { return _cmdbuf_transfer; }
} // namespace vuh
