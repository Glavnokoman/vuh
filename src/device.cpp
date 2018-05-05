#include <vuh/device.h>

#include <cassert>
#include <limits>
#define ARR_VIEW(x) uint32_t(x.size()), x.data()

namespace {
	// create logical device to interact with the physical one
	auto createDevice(const vk::PhysicalDevice& physicalDevice, const std::vector<const char*>& layers
	                  , uint32_t compute_family_id
	                  , uint32_t transfer_family_id
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
		auto devCI = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), n_queues, queueCIs.data()
		                                  , ARR_VIEW(layers));

		return physicalDevice.createDevice(devCI, nullptr);
	}

	/// @return preffered family id for the desired queue flags combination, or -1 if none is found.
	auto getFamilyID(const std::vector<vk::QueueFamilyProperties>& queue_families, vk::QueueFlags tgtFlag)-> uint32_t {
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

	///
	auto allocCmdBuffer(vk::Device device, vk::CommandPool pool
	                    , vk::CommandBufferLevel level=vk::CommandBufferLevel::ePrimary
	                    )-> vk::CommandBuffer
	{
		auto commandBufferAI = vk::CommandBufferAllocateInfo(pool, level, 1); // 1 is the command buffer count here
		return device.allocateCommandBuffers(commandBufferAI)[0];
	}
} // namespace

namespace vuh {
	///
	Device::Device(vk::PhysicalDevice physdevice, std::vector<const char*> layers)
	   : Device(physdevice, layers, physdevice.getQueueFamilyProperties())
	{}

	/// helper constructor
	Device::Device(vk::PhysicalDevice physdevice, std::vector<const char*> layers
	              , const std::vector<vk::QueueFamilyProperties>& familyProperties
	              )
	   : Device(physdevice, layers, getFamilyID(familyProperties, vk::QueueFlagBits::eCompute)
	            , getFamilyID(familyProperties, vk::QueueFlagBits::eTransfer))
	{}

	/// helper constructor
	Device::Device(vk::PhysicalDevice physdevice, std::vector<const char*> layers
	               , uint32_t computeFamilyId, uint32_t transferFamilyId
	               )
	  : _dev(createDevice(physdevice, layers, computeFamilyId, transferFamilyId))
	  , _physdev(physdevice)
	  , _cmdpool_compute(_dev.createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer
	                                             , computeFamilyId}))
	  , _cmdbuf_compute(allocCmdBuffer(_dev, _cmdpool_compute))
	  , _layers(layers)
	  , _cmp_family_id(computeFamilyId)
	  , _tfr_family_id(transferFamilyId)
	{
	}

	/// release resources associated with device
	auto Device::release() noexcept-> void {
		if(_dev){
			_dev.destroyCommandPool(_cmdpool_compute);
			if(_tfr_family_id != _cmp_family_id){
				_dev.destroyCommandPool(_cmdpool_transfer);
			}
			_dev.destroy();
		}
	}

	/// Release resources associated with a logical device
	Device::~Device() noexcept {
		release();
	}

	/// Copy constructor. Creates new handle to the same physical device, and recreates associated pools
	Device::Device(const Device& other)
	   : Device(other._physdev, other._layers, other._cmp_family_id, other._tfr_family_id)
	{}

	/// Copy assignment.
	auto Device::operator=(Device other)-> Device& {
		swap(*this, other);
		return *this;
	}

	/// move constructor.
	Device::Device(Device&& other) noexcept
	   : _dev(other._dev)
	   , _physdev(other._physdev)
	   , _cmdpool_compute(other._cmdpool_compute)
	   , _cmdbuf_compute(other._cmdbuf_compute)
	   , _cmdpool_transfer(other._cmdpool_transfer)
	   , _cmdbuf_transfer(other._cmdbuf_transfer)
	   , _layers(std::move(other._layers))
	   , _cmp_family_id(other._cmp_family_id)
	   , _tfr_family_id(other._tfr_family_id)
	{
		other._dev = nullptr;
	}

	/// move assignment
	auto Device::operator=(Device&& o) noexcept-> Device& {
		swap(*this, o);
		o._dev = nullptr;
		return *this;
	}

	/// Swap the guts of two devices (member-wise)
	auto swap(Device& d1, Device& d2)-> void {
		using std::swap;
		swap(d1._dev, d2._dev);
		swap(d1._physdev         , d2._physdev         );
		swap(d1._cmdpool_compute , d2._cmdpool_compute );
		swap(d1._cmdbuf_compute  , d2._cmdbuf_compute  );
		swap(d1._cmdpool_transfer, d2._cmdpool_transfer);
		swap(d1._cmdbuf_transfer , d2._cmdbuf_transfer );
		swap(d1._layers	          , d2._layers          );
		swap(d1._cmp_family_id , d2._cmp_family_id );
		swap(d1._tfr_family_id, d2._tfr_family_id);
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
	/// @return id of the suitable memory, -1 if no suitable memory found.
	auto Device::selectMemory(vk::Buffer buffer, vk::MemoryPropertyFlags properties
	                          ) const-> uint32_t
	{
		auto memProperties = _physdev.getMemoryProperties();
		auto memoryReqs = _dev.getBufferMemoryRequirements(buffer);
		for(uint32_t i = 0; i < memProperties.memoryTypeCount; ++i){
			if( (memoryReqs.memoryTypeBits & (1u << i))
			    && ((properties & memProperties.memoryTypes[i].propertyFlags) == properties))
			{
				return i;
			}
		}
		return uint32_t(-1);
	}

	///
	auto Device::computeQueue(uint32_t i)-> vk::Queue {
		return _dev.getQueue(_cmp_family_id, i);
	}

	/// Create shader module from binary shader code.
	auto Device::createShaderModule(const std::vector<char>& code, vk::ShaderModuleCreateFlags flags
	                                )-> vk::ShaderModule
	{
		return _dev.createShaderModule({flags, code.size()
		                                , reinterpret_cast<const uint32_t*>(code.data())});
	}

	/// Create pipeline cache
	auto Device::createPipeCache(vk::PipelineCacheCreateInfo info)-> vk::PipelineCache {
		return _dev.createPipelineCache(info);
	}

	/// Create compute pipeline with a given layout.
	/// Shader stage info incapsulates the shader and layout&values of specialization constants.
	auto Device::createPipeline(vk::PipelineLayout pipe_layout
	                            , vk::PipelineCache pipe_cache
	                            , vk::PipelineShaderStageCreateInfo shader_stage_info
	                            , vk::PipelineCreateFlags flags
	                            )-> vk::Pipeline
	{
		auto pipelineCI = vk::ComputePipelineCreateInfo(flags
																		, shader_stage_info, pipe_layout);
		return _dev.createComputePipeline(pipe_cache, pipelineCI, nullptr);
		
	}

	/// @return i-th queue in the family supporting transfer commands.
	auto Device::transferQueue(uint32_t i)-> vk::Queue {
		return _dev.getQueue(_tfr_family_id, i);
	}

	/// Create buffer on a device. Does NOT allocate memory.
	auto Device::makeBuffer(uint32_t size ///< size of buffer in bytes
	                       , vk::BufferUsageFlags usage
	                       )-> vk::Buffer
	{
		auto bufferCI = vk::BufferCreateInfo(vk::BufferCreateFlags(), size, usage);
		return _dev.createBuffer(bufferCI);
	}

	/// Allocate memory on device on the heap with given id
	auto Device::alloc(vk::Buffer buf, uint32_t memory_id)-> vk::DeviceMemory {
	   auto memoryReqs = _dev.getBufferMemoryRequirements(buf);
	   auto allocInfo = vk::MemoryAllocateInfo(memoryReqs.size, memory_id);
		return _dev.allocateMemory(allocInfo);
	}

	/// Bind memory to buffer
	auto Device::bindBufferMemory(vk::Buffer buffer, vk::DeviceMemory memory, uint32_t offset
	                              )-> void
	{
		_dev.bindBufferMemory(buffer, memory, offset);
	}

	/// free memory
	auto Device::freeMemory(vk::DeviceMemory memory)-> void {
		_dev.freeMemory(memory);
	}

	/// Map device memory to get a host accessible pointer. No explicit unmapping required.
	auto Device::mapMemory(vk::DeviceMemory memory
	                       , uint32_t offset
	                       , uint32_t size    ///< size of mapped memory region in bytes
	                       )-> void*
	{
		return _dev.mapMemory(memory, offset, size);
	}

	/// deallocate buffer (does not deallocate associated memory)
	auto Device::destroyBuffer(vk::Buffer buf)-> void {
		_dev.destroyBuffer(buf);
	}

	/// @return handle to command buffer for transfer commands
	auto Device::transferCmdBuffer()-> vk::CommandBuffer& {
		if(!_cmdbuf_transfer){
			assert(!_cmdpool_transfer); // command buffer is supposed to be created together with the command pool
			if(_tfr_family_id == _cmp_family_id){
				_cmdpool_transfer = _cmdpool_compute;
			} else {
				_cmdpool_transfer = _dev.createCommandPool(
				                 {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, _tfr_family_id});
			}
			_cmdbuf_transfer = allocCmdBuffer(_dev, _cmdpool_transfer);
		}
		return _cmdbuf_transfer;
	}
} // namespace vuh
