#pragma once

#include "allocatorDevice.hpp"
#include "allocatorTraits.hpp"
#include "bufferDevice.hpp"
#include "bufferHost.hpp"
#include "bufferSpan.hpp"
#include "queue.hpp"
#include "traits.hpp"

#include <algorithm>

namespace vuh {

/// Synchronous copy of the data from the buffer to host iterable
/// using the default transfer queue of the device to which the buffer belongs.
/// @todo add traits check for Buffer template parameter
template<class BufferView_t, class OutputIt>
auto copy(const BufferView_t& view, OutputIt dst)-> OutputIt{
	if(view.host_visible()){
		auto data = view.host_data();
		std::copy(std::begin(data), std::end(data), dst);
	} else {
		using Stage = BufferHost< typename BufferView_t::value_type
		                        , AllocatorDevice<allocator::traits::HostCached>>;
		auto stage = Stage(view.device(), view.size());
		view.device().default_transfer().copy_sync(view, stage);
		std::copy(std::begin(stage), std::end(stage), dst);
	}
	return std::next(dst, view.size());
}

///
template<class BufferView_t, class InputIt>
auto copy(InputIt first, InputIt last, BufferView_t& buf)-> InputIt {
	if(buf.host_visible()){
		auto data = buf.host_data();
		std::copy(first, last, data.begin());
	} else {
		using Stage = BufferHost<typename BufferView_t::value_type
		                        , AllocatorDevice<allocator::traits::HostCoherent>>;
		auto stage = Stage(buf.device(), first, last);
		buf.device().default_transfer().copy_sync(stage, buf);
	}
}


///
template<class BufferView_t, class InputIt, class F>
auto transform(InputIt first, InputIt last, BufferView_t& buf, F&& f)-> InputIt {
	throw "not implemented";
}

///
template<class BufferView_t, class OutputIt, class F>
auto transform(const BufferView_t& buf, OutputIt first, F&& f)-> OutputIt {
	throw "not implemented";
}


///
template<class C, class BufferView_t>
auto to_host(const BufferView_t& buf)-> traits::Iterable<C> {
	throw "not implemented";
}

///
template<class C, class F, class BufferView_t >
auto to_host(const BufferView_t& data, F&& f)-> traits::Iterable<C> {
	throw "not implemented";
}

} // namespace vuh
