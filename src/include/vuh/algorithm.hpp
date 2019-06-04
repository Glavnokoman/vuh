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

/// Synchronous copy of the data from the buffer to host iterable.
/// using the default transfer queue of the device to which the buffer belongs.
template<class T, class OutputIt>
auto copy(const traits::DeviceBuffer<T>& buf, OutputIt dst)-> OutputIt {
	if(buf.host_visible()){
		auto hd = buf.host_data();
		std::copy(std::begin(hd) + buf.offset(), std::begin(hd) + buf.offset() + buf.size(), dst);
	} else {
		using Stage = BufferHost< typename T::value_type
		                        , AllocatorDevice<allocator::traits::HostCached>>;
		auto stage = Stage(buf.device(), buf.size());
		VUH_CHECKOUT_RET(dst);
		buf.device().default_transfer().copy(buf, stage).submit();
		VUH_CHECKOUT_RET(dst);
		std::copy(std::begin(stage), std::end(stage), dst);
	}
	return std::next(dst, buf.size());
}

/// Synchronous copy of the data from host range to device buffer.
/// using the default transfer queue of the device on which the buffer was allocated.
/// Buffer is supposed to have sufficient memory to accomodate that data.
template<class T, class InputIt>
auto copy(InputIt first, InputIt last, traits::DeviceBuffer<T>& buf)-> void {
	assert(std::distance(first, last) <= buf.size());
	if(buf.host_visible()){
		auto data = buf.host_data();
		std::copy(first, last, data.begin() + buf.offset());
	} else {
		using Stage = BufferHost< typename T::value_type
		                        , AllocatorDevice<allocator::traits::HostCoherent>>;
		auto stage = Stage(buf.device(), first, last);
		VUH_CHECKOUT();
		buf.device().default_transfer().copy(stage, buf).submit();
	}
}

/// Async copy of host data to device memory with host-side synchronization.
/// Blocks initially while copying the data to the stage buffer.
/// Then returns the synchronization token and performs buffer to buffer transfer asynchronously.
template<class T, class InputIt>
auto copy_async(InputIt first, InputIt last, traits::DeviceBuffer<T>&& buf)-> SyncTokenHost {
	assert(std::distance(first, last) <= buf.size());
	if(buf.host_visible()){
		auto data = buf.host_data();
		std::copy(first, last, data.begin() + buf.offset());
		return SyncTokenHost(nullptr);
	} else {
		using Stage = BufferHost<typename std::decay_t<T>::value_type
		                        , AllocatorDevice<allocator::traits::HostCoherent>>;
		auto stage = Stage(buf.device(), first, last);
		VUH_CHECKOUT();
		return buf.device().default_transfer().copy(stage, buf).hb(std::move(stage));
	}
}

/// Async copy between two device buffers with host-side syncronization.
template<class T1, class T2>
auto copy_async(traits::DeviceBuffer<T1>&& src, traits::DeviceBuffer<T2>&& dst)-> SyncTokenHost {
	assert(src.device() == dst.device());
	return src.device().default_transfer().copy(src, dst).hb();
}

///
template<class T, class OutputIt, class F>
auto transform(const traits::DeviceBuffer<T>& buf, OutputIt first, F&& f)-> OutputIt {
	if(buf.host_visible()){
		auto data = buf.host_data();
		std::transform(std::begin(data), std::end(data), first, std::forward<F>(f));
	} else {
		using Stage = BufferHost< typename T::value_type
		                        , AllocatorDevice<allocator::traits::HostCached>>;
		auto stage = Stage(buf.device(), buf.size());
		VUH_CHECKOUT_RET(first);
		buf.device().default_transfer().copy(buf, stage).submit();
		VUH_CHECKOUT_RET(first);
		std::transform(std::begin(stage), std::end(stage), first, std::forward<F>(f));
	}
	return std::next(first, buf.size());
}

///
template<class T, class InputIt, class F>
auto transform(InputIt first, InputIt last, traits::DeviceBuffer<T>& buf, F&& f)-> void {
	assert(std::distance(first, last) <= buf.size());
	if(buf.host_visible()){
		auto data = buf.host_data();
		std::transform(first, last, std::begin(data), std::forward<F>(f));
	} else {
		using Stage = BufferHost<typename T::value_type
		                        , AllocatorDevice<allocator::traits::HostCoherent>>;
		auto stage = Stage(buf.device(), first, last, std::forward<F>(f));
		VUH_CHECKOUT();
		buf.device().default_transfer().copy(stage, buf).submit();
	}
}

///
template<class C, class T>
auto to_host(const traits::DeviceBuffer<T>& buf)-> traits::Iterable<C> {
	auto ret = C(buf.size());
	copy(buf, std::begin(ret));
	return ret;
}

///
template<class C, class F, class T>
auto to_host(const traits::DeviceBuffer<T>& buf, F&& f)-> traits::Iterable<C> {
	auto ret = C(buf.size());
	transform(buf, std::begin(ret), std::forward<F>(f));
	return ret;
}

} // namespace vuh
