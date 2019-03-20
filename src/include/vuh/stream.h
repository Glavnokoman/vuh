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
	template<class... As> auto copy(As&&...)-> Stream&; // placeholder, precise signature TBD
	template<class P> auto run(P& p)-> Stream&;         // placeholder, precise signature TBD

	auto tb()-> vuh::BarrierTransfer;
	auto cb()-> vuh::BarrierCompute;
	auto qb()-> vuh::BarrierQueue;
	auto hb()-> vuh::BarrierHost;
	auto wait()-> void;
}; // class Stream
} // namespace vuh
