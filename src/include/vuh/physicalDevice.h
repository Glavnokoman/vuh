#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {

class Device;
class QueueFamily;

/// construction options for compute device
namespace queueOptions {
	enum Simple {Default, AllQueues};

	/// doc me
	struct Streams {
		const unsigned number;
	};

	/// doc me
	struct Spec {
		unsigned family_id;
		unsigned num_queues;
		std::vector<float> priorities;
	};

	using Specs = std::vector<Spec>;
} // namespace devOpts

/// doc me
class PhysicalDevice: public vk::PhysicalDevice {
public:
	explicit PhysicalDevice();

	auto queueFamilies() const-> std::vector<QueueFamily>;

	auto computeDevice()-> vuh::Device;
	auto computeDevice(queueOptions::Simple)-> vuh::Device;
	auto computeDevice(queueOptions::Streams)-> vuh::Device;
	auto computeDevice(const queueOptions::Specs& specs)-> vuh::Device;

	auto streamCount() const-> std::uint32_t;
private: // data
}; // class PhysicalDevice

} // namespace vuh
