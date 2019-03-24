#include <vuh/physicalDevice.h>
#include <vuh/device.h>

namespace vuh {
/// doc me
auto PhysicalDevice::queueFamilies() const-> std::vector<QueueFamily> {
	throw "not implemented";
}

/// doc me
auto PhysicalDevice::computeDevice()-> vuh::Device {
	throw "not implemented";
}

/// doc me
auto PhysicalDevice::computeDevice(queueOptions::Simple)-> vuh::Device {
	throw "not implemented";
}

/// doc me
auto PhysicalDevice::computeDevice(queueOptions::Streams)-> vuh::Device {
	throw "not implemented";
}

/// doc me
auto PhysicalDevice::computeDevice(const queueOptions::Specs& specs)-> vuh::Device {
	throw "not implemented";
}

/// return max number of "streams" the device can support
auto PhysicalDevice::streamCount() const-> uint32_t {
	throw "not implemented";
}


} // namespace vuh
