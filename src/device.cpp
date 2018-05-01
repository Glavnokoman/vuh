#include <vuh/device.h>

#include <limits>
#define ARR_VIEW(x) uint32_t(x.size()), x.data()

namespace {
	// create logical device to interact with the physical one
	auto createDevice(const vk::PhysicalDevice& physicalDevice, const std::vector<const char*>& layers
	                  , uint32_t computeFamilyID
	                  , uint32_t transferFamilyID
	                  )-> vk::Device
	{
		// When creating the device specify what queues it has
		auto p = float(1.0); // queue priority
		auto queueCIs = std::array<vk::DeviceQueueCreateInfo, 2>{};
		queueCIs[0] = vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), computeFamilyID, 1, &p);
		auto numQueues = uint32_t(1);
		if(transferFamilyID != computeFamilyID){
			queueCIs[1] = vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), transferFamilyID, 1, &p);
			numQueues += 1;
		}
		auto devCI = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), numQueues, queueCIs.data(), ARR_VIEW(layers));

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
	  , _cmdpool_compute(_dev.createCommandPool({vk::CommandPoolCreateFlags(), computeFamilyId}))
	  , _layers(layers)
	  , _computeFamilyId(computeFamilyId)
	  , _transferFamilyId(transferFamilyId)
	{}

	/// Release resources associated with a logical device
	Device::~Device() noexcept {
		_dev.destroyCommandPool(_cmdpool_compute);
		_dev.destroy();
	}

	/// Copy constructor. Creates new handle to the same physical device, same layers and queue families.
	Device::Device(const Device& other)
	   : Device(other._physdev, other._layers, other._computeFamilyId, other._transferFamilyId)
	{}

	/// copy operator.
	auto Device::operator=(const Device& other)-> Device& {
		return *this = Device(other);
	}

	/// @return physical device properties
	auto Device::properties() const-> vk::PhysicalDeviceProperties {
		return _physdev.getProperties();
	}

	/// get memory properties of the memory with given id
	auto Device::memoryProperties(uint32_t id) const-> vk::MemoryPropertyFlags {
		return _physdev.getMemoryProperties().memoryTypes[id].propertyFlags;
	}

	/// Find first memory matching desired properties.
	/// @return id of the suitable memory, -1 if no suitable memory found.
	auto Device::selectMemory(vk::Buffer buffer, vk::MemoryPropertyFlags properties) const-> uint32_t {
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

	/// @return i-th queue in the family supporting transfer commands.
	auto Device::transferQueue(uint32_t i)-> vk::Queue  {
		return _dev.getQueue(_computeFamilyId, i);
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
	auto Device::bindBufferMemory(vk::Buffer buffer, vk::DeviceMemory memory, uint32_t offset)-> void {
		_dev.bindBufferMemory(buffer, memory, offset);
	}

	/// free memory
	auto Device::freeMemory(vk::DeviceMemory memory)-> void {
		_dev.freeMemory(memory);
	}

	/// Map device memory to get a host accessible pointer. No explicit unmapping required.
	auto Device::mapMemory(vk::DeviceMemory memory
	                       , uint32_t size    ///< size of mapped memory region in bytes
	                       , uint32_t offset
	                       )-> void*
	{
		return _dev.mapMemory(memory, offset, size);
	}

	/// deallocate buffer (does not deallocate associated memory)
	auto Device::destroyBuffer(vk::Buffer buf)-> void {
		_dev.destroyBuffer(buf);
	}

	/// @return handle to command buffer for transfer commands
	auto Device::transferCmdBuffer()-> vk::CommandBuffer {
		if(!_cmdbuf_transfer){
			// initialize the command buffer and corresponding pool for transfer commands
			throw "not implemented";
		}
		return _cmdbuf_transfer;
	}

} // namespace vuh


