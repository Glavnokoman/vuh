#pragma once

#include "bufferBase.hpp"
#include "bufferSpan.hpp"
#include "error.hpp"

#include <vulkan/vulkan.h>

#include <algorithm>

namespace vuh {

/// Buffer with memory allocated in host-visible space.
/// Allocator template parameter is expected to allocate memory in host-visible GPU memory, mappable
/// for host access.
/// Memory remains mapped during the whole lifetime of an object of this type.
/// Provides Forward iterator + random-access interface.
template<class T, class Alloc>
class BufferHost: public BufferBase<Alloc>, public HostData<T, BufferBase<Alloc>> {
	using Base = BufferBase<Alloc>;
public:
	using value_type = T;
	/// Construct buffer of given size. Associated memory is not initialized with any data.
	BufferHost(const vuh::Device& device      ///< device to create the buffer
	          , std::size_t n_elements        ///< number of elements
	          , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	          , VkBufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	          )
	   : Base(device, n_elements*sizeof(T), flags_memory, flags_buffer)
	   , HostData<T, Base>(*this, n_elements)
	{}

	/// Construct buffer and initialize with a provided value.
	BufferHost(const vuh::Device& device ///< device to create array on
	          , std::size_t n_elements   ///< number of elements
	          , T value             ///< initializer value
	          , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	          , VkBufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	          )
	   : BufferHost(device, n_elements, flags_memory, flags_buffer)
	{
		VUH_CHECKOUT();
		std::fill_n(this->begin(), n_elements, value);
	}

	/// Construct buffer and initialize it from range of values
	template<class InputIt>
	BufferHost( const Device& device                  ///< device to create the buffer
	          , InputIt first, InputIt last           ///< host range to use for initialization
	          , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	          , VkBufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	          )
	   : BufferHost(device, std::distance(first, last), flags_memory, flags_buffer)
	{
		VUH_CHECKOUT();
		std::copy(first, last, this->begin());
	}

	/// doc me
	template<class InputIt, class F>
	BufferHost( const Device& device
	          , InputIt first, InputIt last
	          , F&& f
	          , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	          , VkBufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	          )
	   : BufferHost(device, std::distance(first, last), flags_memory, flags_buffer)
	{
		VUH_CHECKOUT();
		std::transform(first, last, this->begin(), std::forward<F>(f));
	}

	BufferHost(BufferHost&& other)
	   : Base(std::move(other))
	   , HostData<T, Base>(std::move(other))
	{
		HostData<T, Base>::_buffer = this;
	}

	auto host_data()-> HostDataView<T> {return *this;}
	auto host_data() const-> HostDataView<const T> {return *this;}

	auto span(std::size_t offset, std::size_t count)-> BufferSpan<BufferHost> {
		return vuh::span(*this, offset, count);
	}
}; // class BufferHost
} // namespace vuh
