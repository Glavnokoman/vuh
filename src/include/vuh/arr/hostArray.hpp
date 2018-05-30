#pragma once

#include "basicArray.hpp"

#include <algorithm>

namespace vuh {
namespace arr {

/// Array (storage buffer) with an underlying memory allocated in host-visible space.
/// Provides Forward iterator + random-access interface.
/// Memory remains mapped during the whole lifetime of an object of this type.
template<class T, class Alloc>
class HostArray: public BasicArray<Alloc> {
   using Base = BasicArray<Alloc>;
public:
   ///
   HostArray(vuh::Device& device, size_t n_elements
             , vk::MemoryPropertyFlags flags_memory={}
             , vk::BufferUsageFlags flags_buffer={})
      : BasicArray<Alloc>(device, n_elements*sizeof(T), flags_memory, flags_buffer)
      , _ptr(static_cast<T*>(Base::_dev.mapMemory(Base::_mem, 0, n_elements*sizeof(T))))
      , _size(n_elements)
   {}
   
   ///
   HostArray(vuh::Device& device, size_t n_elements, T value
             , vk::MemoryPropertyFlags flags_memory={}
             , vk::BufferUsageFlags flags_buffer={})
      : HostArray(device, n_elements, flags_memory, flags_buffer)
   {
      std::fill_n(begin(), n_elements, value);
   }
   
   ///
   template<class It1, class It2>
   HostArray(vuh::Device& device, It1 head, It2 tail
             , vk::MemoryPropertyFlags flags_memory={}
             , vk::BufferUsageFlags flags_buffer={})
      : HostArray(device, std::distance(head, tail), flags_memory, flags_buffer)
   {
      std::copy(head, tail, begin());
   }

   ///
   HostArray(HostArray&& o): Base(std::move(o)), _ptr(o._ptr), _size(o._size) {o._ptr = nullptr;}
   auto operator=(HostArray&& o)-> HostArray& {
      using std::swap;
      swap(*this, static_cast<Base&>(o));
      swap(_ptr, o._ptr);
      swap(_size, o._size);
   }
   
   ///
   ~HostArray() noexcept {
      if(_ptr) {
         Base::_dev.unmapMemory(Base::_mem);
      }
   }
   
   ///
   auto begin()-> T* { return _ptr; }
   auto begin() const-> const T* { return _ptr;}
   
   ///
   auto end()-> T* { return begin() + size(); }
   auto end() const-> const T* { return begin() + size();}
   
   ///
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
