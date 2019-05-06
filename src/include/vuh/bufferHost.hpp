#pragma once

#include "bufferBase.hpp"
#include "error.hpp"

#include <vulkan/vulkan.h>

#include <algorithm>

namespace vuh {

/// Array with host data exchange interface suitable for memory allocated in host-visible space.
/// Memory allocation and underlying buffer creation is managed by allocator defined by a template parameter.
/// Such allocator is expected to allocate memory in host-visible GPU memory, mappable
/// for host access.
/// Provides Forward iterator + random-access interface.
/// Memory remains mapped during the whole lifetime of an object of this type.
template<class T, class Alloc>
class BufferHost: public BufferBase<Alloc>, public HostData<T, BufferBase<Alloc>> {
	using Base = BufferBase<Alloc>;
public:
	using value_type = T;
	/// Construct object of the class on given device.
	/// Memory is not initialized with any data.
	BufferHost(const vuh::Device& device      ///< device to create array on
	          , std::size_t n_elements        ///< number of elements
	          , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	          , VkBufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	          )
	   : Base(device, n_elements*sizeof(T), flags_memory, flags_buffer)
	   , HostData<T, Base>(*this, n_elements*sizeof(T))
	{}

	/// Construct array on given device and initialize with a provided value.
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

	/// Construct array on given device and initialize it from range of values
	template<class InputIt>
	BufferHost(const Device& device      ///< device to create array on
	          , InputIt first           ///< beginning of initialization range
	          , InputIt last            ///< end (one past end) of initialization range
	          , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	          , VkBufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	          )
	   : BufferHost(device, std::distance(first, last), flags_memory, flags_buffer)
	{
		VUH_CHECKOUT();
		std::copy(first, last, this->begin());
	}

	/// @todo return non-owning host data class by value
	auto host_device()-> HostData<T, Base>& {return *this;}
	auto host_device() const-> HostData<const T, Base>& {return *this;}
}; // class BufferHost
} // namespace vuh
