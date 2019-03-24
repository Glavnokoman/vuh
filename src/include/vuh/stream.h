#pragma once

#include "queue.h"
#include "program.hpp"

namespace vuh {
/// Specifies how the available device queues are assigned to Stream instances
struct StreamSpec {
	std::uint32_t compute_queue_id;  ///< index of the compute queue in the array of queues present on a device (as returned by device.queues())
	std::uint32_t transfer_queue_id; ///< index of the transport queue in the array of queues present on a device (as returned by device.queues())
};

/// Unified (compute-transport) queue-like object.
/// May wrap the single queue with compute and transport capabilities,
/// or two different queues each providing either transport or compute.
class Stream {
public:
	auto tb()-> vuh::BarrierTransfer<Stream>;
	auto cb()-> vuh::BarrierCompute<Stream>;
	auto qb()-> vuh::BarrierQueue;
	auto hb()-> vuh::BarrierHost;
	auto wait()-> void;

	/// copy from host to device
	template<class SrcIter1, class SrcIter2, class T, class Alloc>
	auto copy( SrcIter1 src_begin, SrcIter2 src_end
	         , vuh::ArrayIter<arr::DeviceArray<T, Alloc>> dst_begin
	         )-> Stream&
	{
		throw "not implemented";
	}

	/// copy data from device to host
	template<class ArrayView_t, class DstIter>
	auto copy(ArrayView_t av, DstIter dst_begin)-> Stream& {
		throw "not implemented";
	}

	/// doc me
	template<class P> auto run(const P& p)-> Stream& {
		throw "not implemented";
	}

	/// wait for inter-queue sync tokens to get triggered
	template<class... Bs> auto on(Bs&... tokens)-> Stream& { throw "not implemented"; }
}; // class Stream
} // namespace vuh
