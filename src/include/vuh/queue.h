#pragma once

#include <vulkan/vulkan.hpp>

namespace vuh {

/// Sync point to initiate the transfer operation
class BarrierTransfer{
public:
	template<class... Ts> auto copy(Ts&&...)->vuh::Queue&;
};

/// Sync point to initiate the compute operation
class BarrierCompute {
public:
	template<class P> auto run(P& program)-> vuh::Queue&;
};

class BarrierQueue;
class BarrierHost;

/// doc me
class QueueFamily: public vk::QueueFamilyProperties {
public:
	explicit QueueFamily(uint32_t id, vk::QueueFamilyProperties properties);

	auto canCompute() const-> bool;
	auto canTransfer() const-> bool;
	auto isUnified() const-> bool { return canCompute() && canTransfer(); }
//	auto id() const-> uint32_t;
private: // data
//	const uint32_t _id; ///< family id
}; // class QueueFamily

/// Interface includes all the stuff queues can do, both transfer and compute.
class Queue: public vk::Queue {
public:
	explicit Queue();

	auto canCompute() const-> bool;
	auto canTransfer() const-> bool;
	auto isUnified() const-> bool { return canCompute() && canTransfer(); }

	template<class... Ts> auto copy(Ts... ts)-> Queue&;
	template<class... Ts> auto run(Ts... ts)-> Queue&;

	auto tb()-> vuh::BarrierTransfer;
	auto cb()-> vuh::BarrierCompute;
	auto qb()-> vuh::BarrierQueue;
	auto hb()-> vuh::BarrierHost;
	auto wait()-> void;
private: // data
	const vk::QueueFlags _flags;    ///< queue family flags
//	const uint32_t     _family_id;  ///< id of the family that queue belongs to
	vk::CommandBuffer* _cmd_buffer; ///< command buffer associated with the family
}; // class Queue


} // namespace vuh
