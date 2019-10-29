#pragma once

#include "vuh/mem/allocDevice.hpp"
#include <vuh/device.h>
#include <vulkan/vulkan.hpp>
#include <cassert>

namespace vuh {
namespace arr {

/// Covers basic array functionality. Wraps the SBO buffer.
/// Keeps the data, handles initialization, copy/move, common interface,
/// binding memory to buffer objects, etc...
template<class Alloc>
class BasicArray: public VULKAN_HPP_NAMESPACE::Buffer {
	static constexpr auto descriptor_flags = VULKAN_HPP_NAMESPACE::BufferUsageFlagBits::eStorageBuffer;
public:
	static constexpr auto descriptor_class = VULKAN_HPP_NAMESPACE::DescriptorType::eStorageBuffer;

	/// Construct SBO array of given size in device memory
	BasicArray(vuh::Device& device                     ///< device to allocate array
	           , size_t size_bytes                     ///< desired size in bytes
	           , VULKAN_HPP_NAMESPACE::MemoryPropertyFlags properties={} ///< additional memory property flags. These are 'added' to flags defind by allocator.
	           , VULKAN_HPP_NAMESPACE::BufferUsageFlags usage={}         ///< additional usage flagsws. These are 'added' to flags defined by allocator.
	           )
	   : VULKAN_HPP_NAMESPACE::Buffer(Alloc::makeBuffer(device, size_bytes, descriptor_flags | usage, _result))
	   , _dev(device)
   {
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(VULKAN_HPP_NAMESPACE::Result::eSuccess == _result);
		if (VULKAN_HPP_NAMESPACE::Result::eSuccess == _result) {
			auto alloc = Alloc();
			_mem = alloc.allocMemory(device, *this, properties);
			_flags = alloc.memoryProperties(device);
			_dev.bindBufferMemory(*this, _mem, 0);
		} else {
			release();
		}
#else
      try{
         auto alloc = Alloc();
         _mem = alloc.allocMemory(device, *this, properties);
         _flags = alloc.memoryProperties(device);
         _dev.bindBufferMemory(*this, _mem, 0);
      } catch(std::runtime_error&){ // destroy buffer if memory allocation was not successful
         release();
         throw;
      }
#endif
	}

	/// Release resources associated with current object.
	~BasicArray() noexcept {release();}
   
   BasicArray(const BasicArray&) = delete;
   BasicArray& operator= (const BasicArray&) = delete;

	/// Move constructor. Passes the underlying buffer ownership.
	BasicArray(BasicArray&& other) noexcept
	   : VULKAN_HPP_NAMESPACE::Buffer(other), _mem(other._mem), _flags(other._flags), _dev(other._dev)
	{
		static_cast<VULKAN_HPP_NAMESPACE::Buffer&>(other) = nullptr;
	}

	/// @return underlying buffer
	auto buffer()-> VULKAN_HPP_NAMESPACE::Buffer { return *this; }

	/// @return offset of the current buffer from the beginning of associated device memory.
	/// For arrays managing their own memory this is always 0.
	auto offset() const-> std::size_t { return 0;}

	/// @return reference to device on which underlying buffer is allocated
	auto device()-> vuh::Device& { return _dev; }

	/// @return true if array is host-visible, ie can expose its data via a normal host pointer.
	auto isHostVisible() const-> bool {
		return bool(_flags & VULKAN_HPP_NAMESPACE::MemoryPropertyFlagBits::eHostVisible);
	}

	/// Move assignment. 
	/// Resources associated with current array are released immidiately (and not when moved from
	/// object goes out of scope).
	auto operator= (BasicArray&& other) noexcept-> BasicArray& {
		release();
		_mem = other._mem;
		_flags = other._flags;
		_dev = other._dev;
		reinterpret_cast<VULKAN_HPP_NAMESPACE::Buffer&>(*this) = reinterpret_cast<VULKAN_HPP_NAMESPACE::Buffer&>(other);
		reinterpret_cast<VULKAN_HPP_NAMESPACE::Buffer&>(other) = nullptr;
		return *this;
	}
	
	/// swap the guts of two basic arrays
	auto swap(BasicArray& other) noexcept-> void {
		using std::swap;
		swap(static_cast<VULKAN_HPP_NAMESPACE::Buffer&>(&this), static_cast<VULKAN_HPP_NAMESPACE::Buffer&>(other));
		swap(_mem, other._mem);
		swap(_flags, other._flags);
		swap(_dev, other._dev);
	}
private: // helpers
	/// release resources associated with current BasicArray object
	auto release() noexcept-> void {
		if(static_cast<VULKAN_HPP_NAMESPACE::Buffer&>(*this)) {
			if (bool(_mem)) {
				_dev.freeMemory(_mem);
			}
			_dev.destroyBuffer(*this);
		}
	}
protected: // data
	vuh::Device& 								_dev;               ///< referes underlying logical device
	VULKAN_HPP_NAMESPACE::DeviceMemory          _mem;           ///< associated chunk of device memory
	VULKAN_HPP_NAMESPACE::MemoryPropertyFlags   _flags;  ///< actual flags of allocated memory (may differ from those requested)
	VULKAN_HPP_NAMESPACE::Result                _result;
}; // class BasicArray
} // namespace arr
} // namespace vuh
