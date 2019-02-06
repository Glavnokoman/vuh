#pragma once

#include <vulkan/vulkan.hpp>

namespace vuh {

class BarrierTransfer;
class BarrierCompute;
class BarrierQueue;
class BarrierHost;

/// doc me
class QueueFamily: public vk::QueueFamilyProperties {
public:
	explicit QueueFamily(uint32_t id, vk::QueueFamilyProperties properties);

	auto canCompute() const-> bool;
	auto canTransfer() const-> bool;
	auto id() const-> uint32_t;
private: // data
	const uint32_t _id;
}; // class QueueFamily

/// Interface includes all the stuff queues can do, both transfer and compute.
class Queue: public vk::Queue {
public:
	explicit Queue();

	template<class... Ts> auto copy(Ts... ts)-> Queue&;
	template<class... Ts> auto run(Ts... ts)-> Queue&;
	auto tb()-> vuh::BarrierTransfer;
	auto cb()-> vuh::BarrierCompute;
	auto qb()-> vuh::BarrierQueue;
	auto hb()-> vuh::BarrierHost;

private: // data
	const uint32_t     _family_id;  ///< id of the family that queue belongs to
	vk::CommandBuffer* _cmd_buffer; ///< command buffer associated with the family
}; // class Queue


} // namespace vuh
