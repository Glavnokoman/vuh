#pragma once

#include "arrayProperties.h"
#include "arrayUtils.h"
#include "allocDevice.hpp"
#include "basicArray.hpp"
#include "hostArray.hpp"

#include <vuh/traits.hpp>

#include <algorithm>
#include <cassert>

namespace vuh {
namespace arr {

///
template<class T, class Alloc>
class DeviceOnlyArray: public BasicArray<Alloc> {
public:
	using value_type = T;
   ///
   DeviceOnlyArray(vuh::Device& device, size_t n_elements
	               , vk::MemoryPropertyFlags flags_memory={}
	               , vk::BufferUsageFlags flags_buffer={})
	   : BasicArray<Alloc>(device, n_elements*sizeof(T), flags_memory, flags_buffer)
	   , _size(n_elements)
	{}

	auto size_bytes() const-> uint32_t { return _size*sizeof(T); }
private:
	uint32_t  _size; ///< number of elements
}; // class DeviceOnlyArray

/// Array with an underlying memory allocation not in a device-local space.
/// Objects of this class attempt to allocate memory in device local memory not mappable 
/// for host access. However actual allocation may take place in a host-visible memory.
/// Some functions (like toHost(), fromHost()) switch to using the simplified data exchange methods
/// in that case. Some do not. In any case the interface is determined by the requested memory 
/// allocation strategy, and not by the fallback in case it is triggered.
template<class T, class Alloc>
class DeviceArray: public BasicArray<Alloc>{
	using Base = BasicArray<Alloc>;
public:
	using value_type = T;
   ///
   DeviceArray(vuh::Device& device, size_t n_elements
              , vk::MemoryPropertyFlags flags_memory={}
              , vk::BufferUsageFlags flags_buffer={})
      : Base(device, n_elements*sizeof(T), flags_memory, flags_buffer)
      , _size(n_elements)
   {}
   
   ///
   template<class C, class=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>>
   DeviceArray(vuh::Device& device
               , const C& c
               , vk::MemoryPropertyFlags flags_memory={}
               , vk::BufferUsageFlags flags_buffer={})
      : DeviceArray(device, c.size(), flags_memory, flags_buffer)
   {
      fromHost(begin(c), end(c));
   }
   
   ///
   template<class It1, class It2>
   DeviceArray(vuh::Device& device, It1 begin, It2 end
               , vk::MemoryPropertyFlags flags_memory={}
               , vk::BufferUsageFlags flags_buffer={})
      : DeviceArray(device, std::distance(begin, end), flags_memory, flags_buffer) 
   {
      fromHost(begin, end);
   }
   
   ///
   template<class F>
   DeviceArray(vuh::Device& device, size_t n_elements
               , F&& fun
               , vk::MemoryPropertyFlags flags_memory={}
               , vk::BufferUsageFlags flags_buffer={})
      : DeviceArray(device, n_elements, flags_memory, flags_buffer)
   {
		using std::begin;
      auto stage_buffer = HostArray<T, AllocDevice<properties::HostStage>>(Base::_dev, n_elements);
      auto stage_it = begin(stage_buffer);
      for(size_t i = 0; i < n_elements; ++i, ++stage_it){
         *stage_it = fun(i);
      }
      copyBuf(Base::_dev, stage_buffer, *this, size_bytes());
   }
   
   ///
   template<class It1, class It2>
	auto fromHost(It1 begin, It2 end)-> void {
		if(memoryHostVisible()){
			std::copy(begin, end, host_ptr());
			Base::_dev.unmapMemory(Base::_mem);
		} else { // memory is not host visible, use staging buffer
			auto stage_buf = HostArray<T, AllocDevice<properties::HostStage>>(Base::_dev, begin, end);
			copyBuf(Base::_dev, stage_buf, *this, size_bytes());
		}
	}
   
   ///
   template<class It>
   auto toHost(It copy_to) const-> void {
      if(memoryHostVisible()){
         std::copy_n(host_ptr(), size(), copy_to);
         Base::_dev.unmapMemory(Base::_mem);
      } else {
         using std::begin; using std::end;
         auto stage_buf = HostArray<T, AllocDevice<properties::HostCached>>(Base::_dev, size());
         copyBuf(Base::_dev, *this, stage_buf, size_bytes());
         std::copy(begin(stage_buf), end(stage_buf), copy_to);
      }
   }
   
   ///
   template<class It, class F>
   auto toHost(It copy_to, F&& fun) const-> void {
      if(memoryHostVisible()){
         auto copy_from = host_ptr();
         std::transform(copy_from, copy_from + size(), copy_to, std::forward<F>(fun));
         Base::_dev.unmapMemory(Base::_mem);
      } else {
         using std::begin; using std::end;
         auto stage_buf = HostArray<T, AllocDevice<properties::HostCached>>(Base::_dev, size());
         copyBuf(Base::_dev, *this, stage_buf, size_bytes());
         std::transform(begin(stage_buf), end(stage_buf), copy_to, std::forward<F>(fun));
      }
   }
   
   ///
   template<class It, class F>
   auto toHost(It copy_to, size_t size, F&& fun) const-> void {
      if(memoryHostVisible()){
         auto copy_from = host_ptr();
         std::transform(copy_from, copy_from + size, copy_to, std::forward<F>(fun));
         Base::_dev.unmapMemory(Base::_mem);
      } else {
         using std::begin; using std::end;
         auto stage_buf = HostArray<T, AllocDevice<properties::HostCached>>(Base::_dev, size);
         copyBuf(Base::_dev, *this, stage_buf, size_bytes());
         std::transform(begin(stage_buf), end(stage_buf), copy_to, std::forward<F>(fun));
      }
   }
   
   ///
   template<class C, typename=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>>
   auto toHost() const-> C {
      auto ret = C(size());
      using std::begin;
      toHost(begin(ret));
      return ret;
   }
   
   /// @return number of elements
   auto size() const-> size_t {return _size;}
   
   /// @return size of a memory chunk occupied by array elements 
   /// (not the size of actually allocated chunk, which may be a bit bigger).
   auto size_bytes() const-> uint32_t {return _size*sizeof(T);}
private: // helpers
   auto memoryHostVisible() const-> bool {
      return bool(Base::_flags & vk::MemoryPropertyFlagBits::eHostVisible);
   }
   
   auto host_ptr()-> T* {
      assert(memoryHostVisible());
      return static_cast<T*>(Base::_dev.mapMemory(Base::_mem, 0, size_bytes()));
   }
   
   auto host_ptr() const-> const T* {
      assert(memoryHostVisible());
      return static_cast<const T*>(Base::_dev.mapMemory(Base::_mem, 0, size_bytes()));
   }
private: // data
   size_t _size; ///< number of elements. Actual allocated memory may be a bit bigger than necessary.
}; // class DeviceArray
} // namespace arr
} // namespace vuh
