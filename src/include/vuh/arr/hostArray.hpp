#pragma once

#include "basicArray.hpp"

#include <algorithm>

namespace vuh {
namespace arr {

/// Array with host data exchange interface suitable for memory allocated in host-visible space.
/// Memory allocation and underlying buffer creation is managed by allocator defined by a template parameter.
/// Such allocator is expected to allocate memory in host-visible GPU memory, mappable
/// for host access.
/// Provides Forward iterator + random-access interface.
/// Memory remains mapped during the whole lifetime of an object of this type.
template<class T, class Alloc>
class HostArray: public BasicArray<Alloc> {
	using Base = BasicArray<Alloc>;
public:
	/// Construct object of the class on given device.
	/// Memory is not initialized with any data.
	HostArray(vuh::Device& device  ///< device to create array on
	          , size_t n_elements  ///< number of elements
	          , vk::MemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	          , vk::BufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	                                                                                     )
	   : BasicArray<Alloc>(device, n_elements*sizeof(T), flags_memory, flags_buffer)
	   , _ptr(static_cast<T*>(Base::_dev.mapMemory(Base::_mem, 0, n_elements*sizeof(T))))
	   , _size(n_elements)
	{}

	/// Construct array on given device and initialize with a provided value.
	HostArray( vuh::Device& device ///< device to create array on
	         , size_t n_elements   ///< number of elements
	         , T value             ///< initializer value
	         , vk::MemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	         , vk::BufferUsageFlags flags_buffer={}	   ///< additional (to defined by allocator) buffer usage flags
	         )
	   : HostArray(device, n_elements, flags_memory, flags_buffer)
	{
		std::fill_n(begin(), n_elements, value);
	}

	/// Construct array on given device and initialize it from range of values
	template<class It1, class It2>
	HostArray(vuh::Device& device ///< device to create array on
	         , It1 begin          ///< beginning of initialization range
	         , It2 end            ///< end (one past end) of initialization range
	         , vk::MemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
	         , vk::BufferUsageFlags flags_buffer={}    ///< additional (to defined by allocator) buffer usage flags
	         )
	   : HostArray(device, std::distance(begin, end), flags_memory, flags_buffer)
	{
		std::copy(begin, end, this->begin());
	}

   /// Move constructor.
   HostArray(HostArray&& o): Base(std::move(o)), _ptr(o._ptr), _size(o._size) {o._ptr = nullptr;}
	/// Move operator.
   auto operator=(HostArray&& o)-> HostArray& {
      using std::swap;
      swap(*this, static_cast<Base&>(o));
      swap(_ptr, o._ptr);
      swap(_size, o._size);
   }
   
   /// Destroy array, and release all associated resources.
   ~HostArray() noexcept {
      if(_ptr) {
         Base::_dev.unmapMemory(Base::_mem);
      }
   }
   
   /// Iterator (forward) to start of array values.
   auto begin()-> T* { return _ptr; }
   auto begin() const-> const T* { return _ptr;}
   
   /// Iterator to the end (one past the last element) of array values.
   auto end()-> T* { return begin() + size(); }
   auto end() const-> const T* { return begin() + size();}
   
   /// Element access operator.
   auto operator[](size_t i)-> T& { return *(begin() + i);}
   auto operator[](size_t i) const-> T { return *(begin() + i);}
   
   /// @return number of elements
   auto size() const-> size_t {return _size;}
   
   /// @return size of a memory chunk occupied by array elements
   /// (not the size of actually allocated chunk, which may be a bit bigger).
   auto size_bytes() const-> uint32_t {return _size*sizeof(T);}
private: // data
   T* _ptr;       ///< host accessible pointer to the beginning of corresponding memory chunk.
   size_t _size;  ///< Number of elements. Actual allocated memory may be a bit bigger then necessary.
}; // class HostArray
} // namespace arr
} // namespace vuh
