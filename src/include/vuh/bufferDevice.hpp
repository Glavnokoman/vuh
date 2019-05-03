#pragma once

#include "allocatorDevice.hpp"
#include "allocatorTraits.hpp"
#include "bufferBase.hpp"
#include "bufferHost.hpp"
#include "traits.hpp"
//#include "arrayUtils.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace vuh {

/// Buffer class not supposed to take part in data exchange with host.
/// The only valid use for such arrays is passing them as (in or out) argument to a shader.
/// Resources allocation is handled by allocator defined by a template parameter which is supposed to provide
/// memory and anderlying vulkan buffer with suitable flags.
template<class T, class Alloc>
class BufferDeviceOnly: public BufferBase<Alloc> {
public:
	using value_type = T;
	/// Constructs object of the class on given device.
	/// Memory is left unintitialized.
	BufferDeviceOnly( vuh::Device& device       ///< deice to create array on
	                , std::uint32_t n_elements  ///< number of elements
	                , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	                , VkBufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	                )
	   : BufferBase<Alloc>(device, n_elements*sizeof(T), flags_memory, flags_buffer)
	   , _size(n_elements)
	{}

	/// @return size of buffer in bytes.
	auto size_bytes() const-> std::uint32_t { return _size*sizeof(T); }
private:
	std::uint32_t _size; ///< number of elements
}; // class BufferDeviceOnly

/// Buffer with host data exchange interface suitable for memory allocated in device-local space.
/// Memory allocation and underlying buffer creation is managed by allocator defined by a template parameter.
/// Such allocator is expected to allocate memory in device local memory not mappable
/// for host access. However actual allocation may take place in a host-visible memory.
/// Some functions (like toHost(), fromHost()) switch to using the simplified data exchange methods
/// in that case. Some do not. In case all memory is host-visible (like on integrated GPUs) using this class
/// may result in performance penalty.
template<class T, class Alloc>
class BufferDevice: public BufferBase<Alloc>{
	using Base = BufferBase<Alloc>;
public:
	using value_type = T;
	/// Create an instance of DeviceArray with given number of elements. Memory is uninitialized.
	BufferDevice( vuh::Device& device        ///< device to create array on
	            , std::size_t n_elements     ///< number of elements
	            , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	            , VkBufferUsageFlags flags_buffer={}   ///< additional (to defined by allocator) buffer usage flags
	            )
	   : Base(device, n_elements*sizeof(T), flags_memory, flags_buffer)
	   , _size(n_elements)
	{}

	/// Create an instance of DeviceArray and initialize it from a range of values.
	template<class It1, class It2>
	BufferDevice(vuh::Device& device   ///< device to create array on
	            , It1 begin           ///< range begin
	            , It2 end             ///< range end (points to one past the last element of the range)
	            , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	            , VkBufferUsageFlags flags_buffer={})   ///< additional (to defined by allocator) buffer usage flags
	   : BufferDevice(device, std::distance(begin, end), flags_memory, flags_buffer)
	{
		fromHost(begin, end); // !
	}

	/// Create an instance of DeviceArray of given size and initialize it using index based initializer function.
	template<class F>
	BufferDevice( vuh::Device& device  ///< device to create array on
	           , size_t n_elements    ///< number of elements
	           , F&& fun              ///< callable of a form function<T(size_t)> mapping an offset to array value
	           , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	           , VkBufferUsageFlags flags_buffer={})   ///< additional (to defined by allocator) buffer usage flags
	   : BufferDevice(device, n_elements, flags_memory, flags_buffer)
	{
		using std::begin;
		using stage_t = BufferHost<T, AllocatorDevice<allocator::traits::HostCoherent>>;
		auto stage_buffer = stage_t(Base::_dev, n_elements);
		auto stage_it = begin(stage_buffer);
		for(size_t i = 0; i < n_elements; ++i, ++stage_it){
			*stage_it = fun(i);
		}
		copyBuf(Base::_dev, stage_buffer, *this, Base::size_bytes());
	}

	/// @return number of elements
	auto size() const-> size_t {return _size;}

	/// @return size of a memory chunk occupied by array elements
	/// (not the size of actually allocated chunk, which may be a bit bigger).
	auto size_bytes() const-> uint32_t {return _size*sizeof(T);}
private: // helpers
	auto host_data()-> T* {
		assert(Base::isHostVisible());
		return static_cast<T*>(Base::_dev.mapMemory(Base::_mem, 0, size_bytes()));
	}

	auto host_data() const-> const T* {
		assert(Base::isHostVisible());
		return static_cast<const T*>(Base::_dev.mapMemory(Base::_mem, 0, size_bytes()));
	}
private: // data
	size_t _size; ///< number of elements. Actual allocated memory may be a bit bigger than necessary.
}; // class DeviceArray


} // namespace vuh
