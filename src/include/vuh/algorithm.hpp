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
template<class DeviceData_t, class OutputIt>
auto copy(const DeviceData_t& ddata, OutputIt dst)-> OutputIt {
	if(ddata.host_visible()){
		auto hd = ddata.host_data();
		std::copy(std::begin(hd), std::end(hd), dst);
	} else {
		using Stage = BufferHost< typename DeviceData_t::value_type
		                        , AllocatorDevice<allocator::traits::HostCached>>;
		auto stage = Stage(ddata.device(), ddata.size());
		VUH_CHECKOUT_RET(dst);
		ddata.device().default_transfer().copy_sync(ddata, stage);
		VUH_CHECKOUT_RET(dst);
		std::copy(std::begin(stage), std::end(stage), dst);
	}
	return std::next(dst, ddata.size());
}

/// Synchronous copy of the data from host range to device buffer
/// using the default transfer queue of the device on which the buffer was allocated.
/// Buffer is supposed to have sufficient memory to accomodate that data.
template<class DeviceData_t, class InputIt>
auto copy(InputIt first, InputIt last, DeviceData_t& buf)-> void {
	assert(std::distance(first, last) <= buf.size());
	if(buf.host_visible()){
		auto data = buf.host_data();
		std::copy(first, last, data.begin());
	} else {
		using Stage = BufferHost<typename DeviceData_t::value_type
		                        , AllocatorDevice<allocator::traits::HostCoherent>>;
		auto stage = Stage(buf.device(), first, last);
		VUH_CHECKOUT();
		buf.device().default_transfer().copy_sync(stage, buf);
	}
}

///
template<class DeviceData_t, class OutputIt, class F>
auto transform(const DeviceData_t& buf, OutputIt first, F&& f)-> OutputIt {
	if(buf.host_visible()){
		auto data = buf.host_data();
		std::transform(std::begin(data), std::end(data), first, std::forward<F>(f));
	} else {
		using Stage = BufferHost< typename DeviceData_t::value_type
		                        , AllocatorDevice<allocator::traits::HostCached>>;
		auto stage = Stage(buf.device(), buf.size());
		VUH_CHECKOUT_RET(first);
		buf.device().default_transfer().copy_sync(buf, stage);
		VUH_CHECKOUT_RET(first);
		std::transform(std::begin(stage), std::end(stage), first, std::forward<F>(f));
	}
	return std::next(first, buf.size());
}

///
template<class DeviceData_t, class InputIt, class F>
auto transform(InputIt first, InputIt last, DeviceData_t& buf, F&& f)-> void {
	assert(std::distance(first, last) <= buf.size());
	if(buf.host_visible()){
		auto data = buf.host_data();
		std::transform(first, last, std::begin(data), std::forward<F>(f));
	} else {
		using Stage = BufferHost<typename DeviceData_t::value_type
		                        , AllocatorDevice<allocator::traits::HostCoherent>>;
		auto stage = Stage(buf.device(), first, last, std::forward<F>(f));
		VUH_CHECKOUT();
		buf.device().default_transfer().copy_sync(stage, buf);
	}
}

///
template<class C, class DeviceData_t>
auto to_host(const DeviceData_t& buf)-> traits::Iterable<C> {
	auto ret = C(buf.size());
	copy(buf, std::begin(ret));
	return ret;
}

///
template<class C, class F, class DeviceData_t >
auto to_host(const DeviceData_t& buf, F&& f)-> traits::Iterable<C> {
	auto ret = C(buf.size());
	transform(buf, std::begin(ret), std::forward<F>(f));
	return ret;
}

} // namespace vuh
