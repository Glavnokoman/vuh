#pragma once

#include <vulkan/vulkan.hpp>

namespace vuh {

class Queue;

/// Sync point to initiate the transfer operation using the same queue that initially issued it.
template<class Q>
class BarrierTransfer{
public:
	template<class... Ts> auto copy(Ts&&...)->Q& {
		throw "not implemented";
	}
};

/// Sync point to initiate the compute operation using the same queue that initially issued it.
template<class Q>
class BarrierCompute {
public:
	template<class P> auto run(P& program)-> Q& {
		throw "not implemented";
	}
};

/// Token used to synchronize operations between different queues
class BarrierQueue{
	explicit BarrierQueue(){ throw "not implemented"; }
};

/// Token to use for host-device synchronization
class BarrierHost {
public:
	auto wait() const { throw "not implemented"; }
};

/// doc me
class QueueFamily: public vk::QueueFamilyProperties {
public:
	explicit QueueFamily(uint32_t id, vk::QueueFamilyProperties properties);

	auto canCompute() const-> bool { throw "not implemented"; }
	auto canTransfer() const-> bool { throw "not implemented"; }
	auto isUnified() const-> bool { return canCompute() && canTransfer(); }
//	auto id() const-> uint32_t;
private: // data
//	const uint32_t _id; ///< family id
}; // class QueueFamily

/// Interface includes all the stuff queues can do, both transfer and compute.
class Queue: public vk::Queue {
public:
	explicit Queue();

	auto canCompute() const-> bool { throw "not implemented"; }
	auto canTransfer() const-> bool { throw "not implemented"; }
	auto isUnified() const-> bool { return canCompute() && canTransfer(); }

	template<class... Ts> auto copy(Ts... ts)-> Queue&;

	template<class... Ts> auto run(Ts... ts)-> Queue&;

	auto tb()-> vuh::BarrierTransfer<Queue>;
	auto cb()-> vuh::BarrierCompute<Queue>;
	auto qb()-> vuh::BarrierQueue;
	auto hb()-> vuh::BarrierHost;
	auto wait()-> void;
private: // data
	const vk::QueueFlags _flags;    ///< queue family flags
//	const uint32_t     _family_id;  ///< id of the family that queue belongs to
	vk::CommandBuffer* _cmd_buffer; ///< command buffer associated with the family
}; // class Queue


} // namespace vuh
