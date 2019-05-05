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
class BufferHost: public BufferBase<Alloc> {
	using Base = BufferBase<Alloc>;
public:
	using value_type = T;
	/// Construct object of the class on given device.
	/// Memory is not initialized with any data.
	BufferHost(vuh::Device& device      ///< device to create array on
	          , std::size_t n_elements  ///< number of elements
	          , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	          , VkBufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	          )
	   : Base(device, n_elements*sizeof(T), flags_memory, flags_buffer)
	   , _size(n_elements)
	{
		VUH_CHECKOUT();
		auto err = vkMapMemory(device, Base::_mem, 0, n_elements*sizeof(T), {}, (void**)(&_data));
		if(err != VK_SUCCESS){
			Base::release();
			VUH_SIGNAL(err);
		}
	}

	/// Construct array on given device and initialize with a provided value.
	BufferHost( vuh::Device& device ///< device to create array on
	          , std::size_t n_elements   ///< number of elements
	          , T value             ///< initializer value
	          , VkMemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	          , VkBufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	          )
	   : BufferHost(device, n_elements, flags_memory, flags_buffer)
	{
		VUH_CHECKOUT();
		std::fill_n(begin(), n_elements, value);
	}

	/// Construct array on given device and initialize it from range of values
	template<class InputIt>
	BufferHost(vuh::Device& device      ///< device to create array on
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

	BufferHost(BufferHost&& o) noexcept
	   : Base(std::move(o)), _data(o._data), _size(o._size)
	{
		o._data = nullptr;
	}
	auto operator=(BufferHost&& o)=delete;

	/// Destroy array, and release all associated resources.
	~BufferHost() noexcept {
		if(_data) {
			vkUnmapMemory(Base::_device, Base::_mem);
		}
	}

	/// Host-accesible iterator to beginning of array data
	auto begin()-> value_type* { return _data; }
	auto begin() const-> const value_type* { return _data; }

	/// Pointer (host) to beginning of array data
	auto data()-> T* {return _data;}
	auto data() const-> const T* {return _data;}

	/// Host-accessible iterator to the end (one past the last element) of array data.
	auto end()-> value_type* { return begin() + size(); }
	auto end() const-> const value_type* { return begin() + size(); }

	/// Element access operator (host-side).
	auto operator[](std::size_t i)-> T& { return *(begin() + i);}
	auto operator[](std::size_t i) const-> T { return *(begin() + i);}

	/// @return number of elements
	auto size() const-> std::size_t {return _size;}

	/// @return size of a memory chunk occupied by array elements
	/// (not the size of actually allocated chunk, which may be a bit bigger).
	auto size_bytes() const-> std::size_t {return _size*sizeof(T);}
private: // data
	T* _data;       ///< host accessible pointer to the beginning of corresponding memory chunk.
	size_t _size;   ///< Number of elements. Actual allocated memory may be a bit bigger then necessary.
}; // class BufferHost
} // namespace vuh
