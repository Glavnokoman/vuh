#pragma once

#include "bufferBase.hpp"
#include "bufferSpan.hpp"
#include "device.hpp"
#include "error.hpp"

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
	BufferDevice(const Device& device        ///< device to create array on
	            , std::size_t n_elements     ///< number of elements
	            , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	            , VkBufferUsageFlags flags_buffer={}   ///< additional (to defined by allocator) buffer usage flags
	            )
	   : Base(device, n_elements*sizeof(T), flags_memory, flags_buffer)
	   , _size(n_elements)
	{}

	BufferDevice(const Device& device , std::size_t count , T value
	            , VkMemoryPropertyFlags flags_memory={}, VkBufferUsageFlags flags_buffer={});

	template<class InputIt>
	BufferDevice( const Device& device, InputIt begin, InputIt end
	            , VkMemoryPropertyFlags flags_memory={}, VkBufferUsageFlags flags_buffer={});

	template<class InputIt, class F>
	BufferDevice( const Device& device, InputIt first, InputIt last, F&& fun
	            , VkMemoryPropertyFlags flags_memory={}, VkBufferUsageFlags flags_buffer={});



	/// @return number of elements
	auto size() const-> std::size_t {return _size;}

	/// @return size of a memory chunk occupied by array elements
	/// (not the size of actually allocated chunk, which may be a bit bigger).
	auto size_bytes() const-> std::size_t {return _size*sizeof(T);}

	///
	auto span(std::size_t offset, std::size_t count)-> BufferSpan<BufferDevice>{
		return vuh::span(*this, offset, count);
	}

	auto host_data()-> HostData<T, Base> {
		assert(this->host_visible());
		return HostData<T, Base>(*this, size_bytes());
	}
	auto host_data() const-> HostData<const T, const Base> {
		assert(this->host_visible());
		return HostData<const T, const Base>(*this, size_bytes());
	}
private: // data
	std::size_t _size; ///< number of elements. Actual allocated memory may be a bit bigger than necessary.
}; // class BufferDevice
} // namespace vuh
