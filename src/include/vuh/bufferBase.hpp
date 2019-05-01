#pragma once

#include "allocatorDevice.hpp"
#include "device.hpp"

#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.hpp>

#include <cassert>
#include <cstdint>

namespace vuh {
VkDeviceSize a;
/// Wraps the SBO buffer. Covers base buffer functionality.
/// Keeps the data, handles initialization, copy/move, common interface,
/// binding memory to buffer objects, etc...
template<class Alloc>
class BufferBase {
	static constexpr auto descriptor_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
public:
	static constexpr auto descriptor_class = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

	/// Construct SBO array of given size in device memory
	BufferBase(vuh::Device& device                   ///< device to allocate array
	           , std::size_t size_bytes              ///< size in bytes
	           , VkMemoryPropertyFlags properties={} ///< additional memory property flags. These are 'added' to flags defind by allocator.
	           , VkBufferUsageFlags usage={}         ///< additional usage flags. These are 'added' to flags defined by allocator.
	           )
	   : _buffer(Alloc::makeBuffer(device, size_bytes, descriptor_flags | usage))
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
	~BufferBase() noexcept {release();}
   
   BufferBase(const BufferBase&) = delete;
   BufferBase& operator= (const BufferBase&) = delete;

	/// Move constructor. Passes the underlying buffer ownership.
	BufferBase(BufferBase&& other) noexcept
	   : vk::Buffer(other), _mem(other._mem), _flags(other._flags), _dev(other._dev)
	{
		static_cast<vk::Buffer&>(other) = nullptr;
	}

	/// @return underlying buffer
	auto buffer()-> vk::Buffer { return *this; }

	/// @return offset of the current buffer from the beginning of associated device memory.
	/// For arrays managing their own memory this is always 0.
	auto offset() const-> std::size_t { return 0;}

	/// @return reference to device on which underlying buffer is allocated
	auto device()-> vuh::Device& { return _dev; }

	/// @return true if array is host-visible, ie can expose its data via a normal host pointer.
	auto isHostVisible() const-> bool {
		return bool(_flags & vk::MemoryPropertyFlagBits::eHostVisible);
	}

	/// Move assignment. 
	/// Resources associated with current array are released immidiately (and not when moved from
	/// object goes out of scope).
	auto operator= (BufferBase&& other) noexcept-> BufferBase& {
		release();
		_mem = other._mem;
		_flags = other._flags;
		_dev = other._dev;
		reinterpret_cast<vk::Buffer&>(*this) = reinterpret_cast<vk::Buffer&>(other);
		reinterpret_cast<vk::Buffer&>(other) = nullptr;
		return *this;
	}
	
	/// swap the guts of two basic arrays
	auto swap(BufferBase& other) noexcept-> void {
		using std::swap;
		swap(static_cast<vk::Buffer&>(&this), static_cast<vk::Buffer&>(other));
		swap(_mem, other._mem);
		swap(_flags, other._flags);
		swap(_dev, other._dev);
	}
private: // helpers
	/// release resources associated with current BufferBase object
	auto release() noexcept-> void {
		if(static_cast<vk::Buffer&>(*this)){
			_dev.freeMemory(_mem);
			_dev.destroyBuffer(*this);
		}
	}
protected: // data
	VkBuffer _buffer;
	vk::DeviceMemory _mem;           ///< associated chunk of device memory
	vk::MemoryPropertyFlags _flags;  ///< actual flags of allocated memory (may differ from those requested)
	vuh::Device& _dev;               ///< referes underlying logical device
}; // class BufferBase
} // namespace vuh
