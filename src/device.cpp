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
	  , _cmdpool(_dev.createCommandPool({vk::CommandPoolCreateFlags(), computeFamilyId}))
	  , _layers(layers)
	  , _computeFamilyId(computeFamilyId)
	  , _transferFamilyId(transferFamilyId)
	{}

	/// Release resources associated with a logical device
	Device::~Device() noexcept {
		_dev.destroyCommandPool(_cmdpool);
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

} // namespace vuh


