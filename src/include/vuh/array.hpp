#pragma once

#include "device.h"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace vuh {
/// Device buffer owning its chunk of memory.
template<class T> 
class Array {
public:
	using value_type = T;
	
   explicit Array(vuh::Device& device, uint32_t size)
      :_dev(device)
   {
      throw "not implemented";
   }
   
   explicit Array(vuh::Device& device, size_t size)
      : _dev(device)
   {
      throw "not implemented";
   }
   
   ~Array() noexcept;
   
   Array(const Array&) = delete;
   Array& operator= (const Array&) = delete;
   Array(Array&&) = default;
   Array& operator= (Array&&) = default;
   
   template<class C>
   static auto fromHost(const C& c, vuh::Device& device)-> Array {throw "not implemented";}
   
   template<class C>
   auto fromHost(const C& c)-> void {throw "not implemented";}
   
   template<class C>
   auto toHost(C& c)-> void {throw "not implemented";}
   
private: // data
	vk::Buffer _buf;                        ///< device buffer
	vk::DeviceMemory _mem;                  ///< associated chunk of device memorys
	vk::MemoryPropertyFlags _flags;         ///< Actual flags of allocated memory. Can be a superset of requested flags.
	vuh::Device& _dev; ///< pointer to logical device. no real ownership, just to provide value semantics to the class.
	uint32_t _size;                         ///< number of elements. actual allocated memory may be a bit bigger than necessary.
}; // class Array
} // namespace vuh
