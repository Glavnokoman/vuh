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
auto copy_async(InputIt first, InputIt last, traits::DeviceBuffer<T>&& buf){
	assert(std::distance(first, last) <= buf.size());
	using Stage = BufferHost<typename std::decay_t<T>::value_type
	                        , AllocatorDevice<allocator::traits::HostCoherent>>;
	if(buf.host_visible()){
		auto data = buf.host_data();
		std::copy(first, last, data.begin() + buf.offset());
		return SyncTokenHostResource<Stage>{};
	} else {
		auto stage = Stage(buf.device(), first, last);
		VUH_CHECKOUT_RET(SyncTokenHostResource<Stage>{});
		return buf.device().default_transfer().copy(stage, buf).hb(std::move(stage));
	}
}

/// Async copy between two device buffers with host-side syncronization.
template<class T1, class T2>
auto copy_async(traits::DeviceBuffer<T1>&& src, traits::DeviceBuffer<T2>&& dst)-> SyncTokenHost {
	assert(src.device() == dst.device());
	return src.device().default_transfer().copy(src, dst).hb();
}

namespace detail {
	struct IRes{ virtual ~IRes() noexcept = default; };

	template<class T, class OutputIt>
	struct DataRes: IRes {
		T data;
		std::size_t offset, count;
		OutputIt dst;

		DataRes(T&& data, size_t offset, size_t count, OutputIt dst)
		   : data(std::move(data)), offset(offset), count(count), dst(dst)
		{}
		~DataRes() noexcept { std::copy(data.begin() + offset, data.begin() + offset + count, dst); }
	};

	template<class Stage, class OutputIt>
	struct StageRes: IRes {
		Stage stage;
		OutputIt dst;

		StageRes(Stage&& stage, OutputIt dst): stage(std::move(stage)), dst(dst){}
		~StageRes() noexcept {
			auto data = stage.host_data();
			std::copy(data.begin(), data.end(), dst);
		}
	};
} // namespace detail

/// Async copy from device buffer to host iterator.
template<class T, class OutputIt>
auto copy_async(traits::DeviceBuffer<T>&& buf, traits::NotDeviceBuffer<OutputIt> dst)
	-> SyncTokenHostResource<std::unique_ptr<detail::IRes>>
{
	using Stage = BufferHost< typename T::value_type
	                        , AllocatorDevice<allocator::traits::HostCached>>;
	using HD = decltype(std::declval<std::decay_t<T>>().host_data());

	if(buf.host_visible()){
		return { SyncTokenHost(nullptr)
		       , std::make_unique<detail::DataRes<HD, OutputIt>>
		                                         (buf.host_data(), buf.offset(), buf.size(), dst)};
	} else {
		VUH_CHECKOUT_RET(SyncTokenHostResource<std::unique_ptr<detail::IRes>>{});
		auto resptr = new detail::StageRes<Stage, OutputIt>(Stage(buf.device(), buf.size()), dst);
		return buf.device().default_transfer().copy(buf, resptr->stage)
		          .hb(std::unique_ptr<detail::IRes>(resptr));
	}
}

///
template<class T, class OutputIt, class F>
auto transform(const traits::DeviceBuffer<T>& buf, OutputIt first, F&& f)-> OutputIt {
	if(buf.host_visible()){
        auto hd = buf.host_data();
        std::transform( std::begin(hd) + buf.offset(), std::begin(hd) + buf.offset() + buf.size()
                      , first, std::forward<F>(f));
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
        auto hd = buf.host_data();
        std::transform(first, last, std::begin(hd) + buf.offset(), std::forward<F>(f));
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
