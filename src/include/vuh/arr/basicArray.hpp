#pragma once

#include "allocDevice.hpp"

#include <vuh/device.h>

#include <vulkan/vulkan.hpp>

#include <cassert>

namespace vuh {
namespace arr {

/// Base class for arrays wrapping the SBO buffer.
/// Keeps the data, handles initialization, copy/move, common interface, etc...
template<class Alloc>
class BasicArray: public vk::Buffer {
public:
	static constexpr auto descriptor_class = vk::BufferUsageFlagBits::eStorageBuffer;

	/// Construct SBO array of given size in device memory
	BasicArray(vuh::Device& device                     ///< device to allocate array
	           , uint32_t size_bytes                   ///< desired size in bytes
	           , vk::MemoryPropertyFlags properties={} ///< additional memory property flags. These are 'added' to flags defind by allocator.
	           , vk::BufferUsageFlags usage={}         ///< additional usage flagsws. These are 'added' to flags defined by allocator.
	           )
	   : vk::Buffer(Alloc::makeBuffer(device, size_bytes, descriptor_class | usage))
	   , _dev(device)
   {
      try{
         auto alloc = Alloc();
         _mem = alloc.allocMemory(device, *this, properties);
         _flags = alloc.memoryProperties(device);
         _dev.bindBufferMemory(*this, _mem, 0);
      } catch(std::runtime_error&){ // destroy buffer if memory allocation was not successful
         release();
         throw;
      }
	}

	/// Release resources associated with current object.
	~BasicArray() noexcept {release();}
   
   BasicArray(const BasicArray&) = delete;
   BasicArray& operator= (const BasicArray&) = delete;

	/// Move constructor. Passes the underlying buffer ownership.
	BasicArray(BasicArray&& other) noexcept
	   : vk::Buffer(other), _mem(other._mem), _flags(other._flags), _dev(other._dev)
	{
		static_cast<vk::Buffer&>(other) = nullptr;
	}

	/// Move assignment. Resources associated with current array are released immidiately and not when moved from
	/// object goes out of scope.
	auto operator= (BasicArray&& other) noexcept-> BasicArray& {
		release();
		std::memcpy(this, &other, sizeof(BasicArray));
		reinterpret_cast<vk::Buffer&>(other) = nullptr;
		return *this;
	}
	
	/// swap the guts of two basic arrays
	friend auto swap(BasicArray& a1, BasicArray& a2) noexcept-> void {
		using std::swap;
		swap(a1, static_cast<vk::Buffer&>(a2));
		swap(a1._mem, a2._mem);
		swap(a1._flags, a2._flags);
		swap(a1._dev, a2._dev);
	}
private: // helpers
	/// release resources associated with current BasicArray object
	auto release() noexcept-> void {
		if(static_cast<vk::Buffer&>(*this)){
			_dev.freeMemory(_mem);
			_dev.destroyBuffer(*this);
		}
	}
protected: // data
	vk::DeviceMemory _mem;           ///< associated chunk of device memory
	vk::MemoryPropertyFlags _flags;  ///< actual flags of allocated memory (may differ from those requested)
	vuh::Device& _dev;               ///< referes underlying logical device
}; // class BasicArray
} // namespace arr
} // namespace vuh
