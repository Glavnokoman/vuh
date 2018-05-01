#pragma once

#include "device.h"
#include "utils.h"

#include <vulkan/vulkan.hpp>

#include <memory>

namespace vuh {
	namespace detail {
		/// Crutch to modify buffer usage flags to include transfer bit in case those may be needed.
		inline auto update_usage(const vuh::Device& device
		                        , vk::MemoryPropertyFlags properties
		                        , vk::BufferUsageFlags usage
		                        )-> vk::BufferUsageFlags
		{
			if(device.properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu
			   && properties == vk::MemoryPropertyFlagBits::eDeviceLocal
			   && usage == vk::BufferUsageFlagBits::eStorageBuffer)
			{
				usage |= vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
			}
			return usage;
		}
	} // namespace detail

/// Device buffer owning its chunk of memory.
template<class T> 
class Array {
public:
	using value_type = T;
	
	Array(vuh::Device& device, uint32_t n_elements
	      , vk::MemoryPropertyFlags properties=vk::MemoryPropertyFlagBits::eDeviceLocal
	      , vk::BufferUsageFlags usage=vk::BufferUsageFlagBits::eStorageBuffer
	      )
	   : Array(device
	           , device.makeBuffer(n_elements*sizeof(T)
	                               , detail::update_usage(device, properties, usage))
	           , properties, n_elements)
   {}

	~Array() noexcept {release();}
   
   Array(const Array&) = delete;
   Array& operator= (const Array&) = delete;
	/// move constructor. passes the underlying buffer ownership
	Array(Array&& other) noexcept
	   : _buf(other._buf), _mem(other._mem), _flags(other._flags), _dev(other._dev), _size(other._size)
	{
		other._buf = nullptr;
	}

	/// Move assignment. Self move-assignment is UB (as it should be).
	auto operator= (Array&& other) noexcept-> Array& {
		release();
		std::memcpy(this, &other, sizeof(Array));
		other._buf = nullptr;
		return *this;
	}
   
   template<class C>
   static auto fromHost(const C& c, vuh::Device& device)-> Array {throw "not implemented";}
   
   template<class C>
   auto fromHost(const C& c)-> void {throw "not implemented";}
   
   template<class C>
   auto toHost(C& c)-> void {throw "not implemented";}
private: // helpers
	/// Helper constructor
	Array(vuh::Device& device
	     , vk::Buffer buffer
	     , vk::MemoryPropertyFlags properties
	     , uint32_t n_elements
	     )
	   : Array(device, buffer, n_elements, device.selectMemory(buffer, properties))
	{}

	/// Helper constructor. This one does the actual construction and binding.
	Array(Device& device, vk::Buffer buf, uint32_t n_elements, uint32_t mem_id)
	   : _buf(buf)
	   , _mem(device.alloc(buf, mem_id))
	   , _flags(device.memoryProperties(mem_id))
	   , _dev(device)
	   , _size(n_elements)
	{
		device.bindBufferMemory(buf, _mem, 0);
	}
   
	/// release resources associated with current Array object
	auto release() noexcept-> void {
		if(_buf){
			_dev.freeMemory(_mem);
			_dev.destroyBuffer(_buf);
		}
	}
private: // data
	vk::Buffer _buf;                        ///< device buffer
	vk::DeviceMemory _mem;                  ///< associated chunk of device memorys
	vk::MemoryPropertyFlags _flags;         ///< Actual flags of allocated memory. Can be a superset of requested flags.
	vuh::Device& _dev; ///< pointer to logical device. no real ownership, just to provide value semantics to the class.
	uint32_t _size;                         ///< number of elements. actual allocated memory may be a bit bigger than necessary.
}; // class Array
} // namespace vuh
