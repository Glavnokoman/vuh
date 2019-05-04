#pragma once

#include "allocatorDevice.hpp"
#include "allocatorTraits.hpp"
#include "bufferDevice.hpp"
#include "bufferHost.hpp"
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
	throw "not implemented";
}


/// Create an instance of DeviceArray and initialize it from a range of values.
template<class T, class Alloc>
template<class InputIt>
BufferDevice<T, Alloc>::BufferDevice(vuh::Device& device  ///< device to create array on
                                    , InputIt begin       ///< range begin
                                    , InputIt end         ///< range end (points to one past the last element of the range)
                                    , VkMemoryPropertyFlags flags_memory  ///< additional (to defined by allocator) memory usage flags
                                    , VkBufferUsageFlags flags_buffer     ///< additional (to defined by allocator) buffer usage flags
                                    )
   : BufferDevice(device, std::distance(begin, end), flags_memory, flags_buffer)
{
	// copy the data from host here
	throw "not implemented";
}

/// Create an instance of DeviceArray of given size and initialize it using index based initializer function.
template<class T, class Alloc>
template<class F>
BufferDevice<T, Alloc>::BufferDevice( vuh::Device& device  ///< device to create array on
            , size_t n_elements    ///< number of elements
            , F&& fun              ///< callable of a form function<T(size_t)> mapping an offset to array value
            , VkMemoryPropertyFlags flags_memory  ///< additional (to defined by allocator) memory usage flags
            , VkBufferUsageFlags flags_buffer     ///< additional (to defined by allocator) buffer usage flags
            )
   : BufferDevice(device, n_elements, flags_memory, flags_buffer)
{
	using std::begin;
	using stage_t = BufferHost<T, AllocatorDevice<allocator::traits::HostCoherent>>;
	auto stage_buffer = stage_t(Base::_dev, n_elements);
	auto stage_it = begin(stage_buffer);
	for(size_t i = 0; i < n_elements; ++i, ++stage_it){
		*stage_it = fun(i);
	}
	throw "not implemented";
//	copyBuf(Base::_dev, stage_buffer, *this, Base::size_bytes());
}

} // namespace vuh
