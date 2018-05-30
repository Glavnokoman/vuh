#include <vuh/device.h>

#include <cassert>
#include <limits>
#define ARR_VIEW(x) uint32_t(x.size()), x.data()

namespace {
	/// Create logical device.
	auto createDevice(const vk::PhysicalDevice& physicalDevice
	                  , const std::vector<const char*>& layers
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
	auto getFamilyID(const std::vector<vk::QueueFamilyProperties>& queue_families
	                 , vk::QueueFlags tgtFlag
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
	  : vk::Device(createDevice(physdevice, layers, computeFamilyId, transferFamilyId))
	  , _physdev(physdevice)
	  , _layers(layers)
	  , _cmp_family_id(computeFamilyId)
	  , _tfr_family_id(transferFamilyId)
	{
		try {
			_cmdpool_compute = createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer
			                                     , computeFamilyId});
			_cmdbuf_compute = allocCmdBuffer(*this, _cmdpool_compute);
		} catch(vk::Error&) {
			release(); // because vk::Device does not know how to clean after itself
			throw;
		}
	}

	/// release resources associated with device
	auto Device::release() noexcept-> void {
		if(static_cast<vk::Device&>(*this)){
			destroyCommandPool(_cmdpool_compute);
			if(_tfr_family_id != _cmp_family_id){
				destroyCommandPool(_cmdpool_transfer);
			}
			vk::Device::destroy();
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

	/// Move constructor.
	Device::Device(Device&& other) noexcept
	   : vk::Device(std::move(other))
	   , _physdev(other._physdev)
	   , _cmdpool_compute(other._cmdpool_compute)
	   , _cmdbuf_compute(other._cmdbuf_compute)
	   , _cmdpool_transfer(other._cmdpool_transfer)
	   , _cmdbuf_transfer(other._cmdbuf_transfer)
	   , _layers(std::move(other._layers))
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
		swap(d1._layers	       , d2._layers          );
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

	///
	auto Device::computeQueue(uint32_t i)-> vk::Queue {
		return getQueue(_cmp_family_id, i);
	}

	/// Create shader module from binary shader code.
	/// @todo remove
	auto Device::createShaderModule(const std::vector<char>& code, vk::ShaderModuleCreateFlags flags
	                                )-> vk::ShaderModule
	{
		return vk::Device::createShaderModule({flags, code.size()
		                                , reinterpret_cast<const uint32_t*>(code.data())});
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

	/// @return i-th queue in the family supporting transfer commands.
	auto Device::transferQueue(uint32_t i)-> vk::Queue {
		return getQueue(_tfr_family_id, i);
	}

	/// Allocate device memory for the buffer on the heap with given id.
	auto Device::alloc(vk::Buffer buf, uint32_t memory_id)-> vk::DeviceMemory {
	   auto memoryReqs = getBufferMemoryRequirements(buf);
	   auto allocInfo = vk::MemoryAllocateInfo(memoryReqs.size, memory_id);
		return allocateMemory(allocInfo);
	}

	/// @return handle to command buffer for transfer commands
	auto Device::transferCmdBuffer()-> vk::CommandBuffer& {
		if(!_cmdbuf_transfer){
			assert(!_cmdpool_transfer); // command buffer is supposed to be created together with the command pool
			if(_tfr_family_id == _cmp_family_id){
				_cmdpool_transfer = _cmdpool_compute;
			} else {
				_cmdpool_transfer = createCommandPool(
				                 {vk::CommandPoolCreateFlagBits::eResetCommandBuffer, _tfr_family_id});
			}
			_cmdbuf_transfer = allocCmdBuffer(*this, _cmdpool_transfer);
		}
		return _cmdbuf_transfer;
	}
} // namespace vuh
