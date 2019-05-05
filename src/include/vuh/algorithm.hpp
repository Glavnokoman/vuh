#pragma once

#include "allocatorDevice.hpp"
#include "allocatorTraits.hpp"
#include "bufferDevice.hpp"
#include "bufferHost.hpp"
#include "bufferSpan.hpp"
#include "queue.hpp"

#include "traits.hpp"

namespace vuh {

/// Synchronous copy of the data from the buffer to host iterable
/// using the default transfer queue of the device to which the buffer belongs.
template<class BufferView_t, class OutputIt>
auto copy(const BufferView_t& view, OutputIt dst)-> OutputIt{
	if(view.host_visible()){
		const auto& data = view.host_data();
		std::copy(std::begin(data), std::end(data), dst);
	} else {
		using Stage = BufferHost< typename BufferView_t::value_type
		                        , AllocatorDevice<allocator::traits::HostCached>>;
		auto stage = Stage(view.device(), view.size());
		view.device().default_transfer().copy_sync(view, stage);
		std::copy(std::begin(stage), std::end(stage), dst);
	}
}

///
template<class T, class Alloc, class InputIt>
auto copy(InputIt first, InputIt last, BufferDevice<T, Alloc>& buf)-> InputIt;

///
template<class T, class Alloc, class InputIt>
auto copy(InputIt first, InputIt last, BufferSpan<BufferDevice<T, Alloc>> data)-> InputIt;

///
template<class T, class Alloc, class InputIt>
auto copy(InputIt first, InputIt last, BufferHost<T, Alloc>& buf)-> InputIt;

///
template<class T, class Alloc, class InputIt>
auto copy(InputIt first, InputIt last, BufferSpan<BufferHost<T, Alloc>> data)-> InputIt;


///
template<class T, class Alloc, class InputIt, class F>
auto transform(InputIt first, InputIt last, BufferDevice<T, Alloc>& buf, F&& f)-> InputIt;

///
template<class T, class Alloc, class InputIt, class F>
auto transform(InputIt first, InputIt last, BufferSpan<BufferDevice<T, Alloc>> data, F&& f)-> InputIt;

///
template<class T, class Alloc, class OutputIt, class F>
auto transform(const BufferDevice<T, Alloc>& buf, OutputIt first, F&& f)-> OutputIt;

///
template<class T, class Alloc, class OutputIt, class F>
auto transform(BufferSpan<BufferDevice<T, Alloc>> data, OutputIt first, F&& f)-> OutputIt;


///
template<class C, class T, class Alloc
        , typename=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>
        >
auto to_host(BufferSpan<BufferDevice<T, Alloc>> data)-> C;

///
template<class C, class T, class Alloc
        , typename=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>
        >
auto to_host(const BufferDevice<T, Alloc>& buf)-> C;

///
template<class C, class F, class T, class Alloc
        , typename=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>
        >
auto to_host(BufferSpan<BufferDevice<T, Alloc>> data, F&& f)-> C;

///
template<class C, class F, class T, class Alloc
        , typename=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>
        >
auto to_host(const BufferDevice<T, Alloc>& buf, F&& f)-> C;



} // namespace vuh
