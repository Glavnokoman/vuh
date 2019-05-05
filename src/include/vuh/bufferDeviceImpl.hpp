#pragma once

#include "allocatorDevice.hpp"
#include "allocatorTraits.hpp"
#include "bufferDevice.hpp"
#include "bufferHost.hpp"
#include "queue.hpp"
#include "traits.hpp"

#include <vector>

namespace vuh {

///
template<class T, class Alloc>
BufferDevice<T, Alloc>::BufferDevice( Device& device
                                    , std::size_t count
                                    , T value
                                    , VkMemoryPropertyFlags flags_memory
                                    , VkBufferUsageFlags flags_buffer
                                    )
	: BufferDevice(device, count, flags_memory, flags_buffer)
{
	if(Base::host_visible()){
		auto data = this->host_data();
		std::fill_n(std::begin(data), count, value);
	} else {
		using stage_t = BufferHost<T, AllocatorDevice<allocator::traits::HostCoherent>>;
		auto stage_buffer = stage_t(device, count, value);
		VUH_CHECKOUT();
		device.default_transfer().copy_sync(stage_buffer, *this);
	}
}


/// Create an instance of DeviceArray and initialize it from a range of values.
template<class T, class Alloc>
template<class InputIt>
BufferDevice<T, Alloc>::BufferDevice(vuh::Device& device  ///< device to create array on
                                    , InputIt first       ///< range begin
                                    , InputIt last        ///< range end (points to one past the last element of the range)
                                    , VkMemoryPropertyFlags flags_memory  ///< additional (to defined by allocator) memory usage flags
                                    , VkBufferUsageFlags flags_buffer     ///< additional (to defined by allocator) buffer usage flags
                                    )
   : BufferDevice(device, std::distance(first, last), flags_memory, flags_buffer)
{
	// copy the data from host here
	if(this->host_visible()){
		std::copy(first, last, host_data().data());
	} else {
		using stage_t = BufferHost<T, AllocatorDevice<allocator::traits::HostCoherent>>;
		auto stage_buffer = stage_t(device, first, last);
		VUH_CHECKOUT();
		device.default_transfer().copy_sync(stage_buffer, *this);
	}
}

/// Create an instance of DeviceArray of given size and initialize it using index based initializer function.
template<class T, class Alloc>
template<class InputIt, class F>
BufferDevice<T, Alloc>::BufferDevice(vuh::Device& device  ///< device to create array on
            , InputIt first
            , InputIt last
            , F&& fun                             ///< callable of a form function<T(size_t)> mapping an offset to array value
            , VkMemoryPropertyFlags flags_memory  ///< additional (to defined by allocator) memory usage flags
            , VkBufferUsageFlags flags_buffer     ///< additional (to defined by allocator) buffer usage flags
            )
   : BufferDevice(device, first, flags_memory, flags_buffer)
{
	if(this->host_visible()){
		std::transform(first, last, host_data().data(), fun);
	} else {
		using stage_t = BufferHost<T, AllocatorDevice<allocator::traits::HostCoherent>>;
		auto stage_buffer = stage_t(Base::_device, first, last, fun);
		VUH_CHECKOUT();
		device.default_transfer().copy_sync(stage_buffer, *this);
	}
}

} // namespace vuh
