#include <vuh/stream.h>

namespace vuh {
///
auto Stream::tb()-> vuh::BarrierTransfer<Stream> {
	throw "not implemented";
}

///
auto Stream::cb()-> vuh::BarrierCompute<Stream> {
	throw "not implemented";
}

///
auto Stream::qb()-> vuh::BarrierQueue {
	throw "not implemented";
}

///
auto Stream::hb()-> vuh::BarrierHost {
	throw "not implemented";
}

///
auto Stream::wait()-> void {
	throw "not implemented";
}

} // namespace vuh
