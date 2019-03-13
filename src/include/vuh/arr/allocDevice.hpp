#pragma once

#include <vuh/device.h>
#include <vuh/error.h>
#include <vuh/instance.h>

#include <vulkan/vulkan.hpp>

#include <cassert>

namespace vuh {
namespace arr {

/// Helper class to allocate memory directly from a device memory (incl. host-visible space)
/// (as opposed to allocating from a pool) and initialize the buffer.
/// Binding between memory and buffer is done elsewhere.
template<class Props>
class AllocDevice{
public:
	using properties_t = Props;
	using AllocFallback = AllocDevice<typename Props::fallback_t>; ///< fallback allocator

	/// Create buffer on a device.
	static auto makeBuffer(vuh::Device& device   ///< device to create buffer on
	                      , size_t size_bytes    ///< desired size in bytes
	                      , vk::BufferUsageFlags flags ///< additional (to the ones defined in Props) buffer usage flags
	                      , vk::Result& result
	                      )-> vk::Buffer
	{
		const auto flags_combined = flags | vk::BufferUsageFlags(Props::buffer);

		auto buffer = device.createBuffer({ {}, size_bytes, flags_combined});
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		result = buffer.result;
		VULKAN_HPP_ASSERT(vk::Result::eSuccess == result);
		return buffer.value;
#else
		result = vk::Result::eSuccess;
		return buffer;
#endif
	}

	/// Allocate memory for the buffer.
	auto allocMemory(vuh::Device& device  ///< device to allocate memory
	                 , vk::Buffer buffer  ///< buffer to allocate memory for
	                 , vk::MemoryPropertyFlags flags_memory={} ///< additional (to the ones defined in Props) memory property flags
	                 )-> vk::DeviceMemory 
	{
		_memid = findMemory(device, buffer, flags_memory);
		auto mem = vk::DeviceMemory{};
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		auto alloc_mem = device.allocateMemory({device.getBufferMemoryRequirements(buffer).size, _memid});
		VULKAN_HPP_ASSERT(vk::Result::eSuccess == alloc_mem.result);
		if (vk::Result::eSuccess == alloc_mem.result) {
			mem = alloc_mem.value;
		} else {
			device.instance().report("AllocDevice failed to allocate memory, using fallback", vk::to_string(alloc_mem.result).c_str()
					, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
#else
		try{
			mem = device.allocateMemory({device.getBufferMemoryRequirements(buffer).size, _memid});
		} catch (vk::Error& e){
			device.instance().report("AllocDevice failed to allocate memory, using fallback", e.what()
			                         , VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
#endif
			auto allocFallback = AllocFallback{};
			mem = allocFallback.allocMemory(device, buffer, flags_memory);
			_memid = allocFallback.memId();
		}

		return mem;
	}

	/// @return memory id on which actual allocation took place.
	auto memId() const-> uint32_t {
		assert(_memid != uint32_t(-1)); // should only be called after successful allocMemory() call
		return _memid;
	}

	/// @return memory property flags of the memory on which actual allocation took place.
	auto memoryProperties(vuh::Device& device) const-> vk::MemoryPropertyFlags {
		return device.memoryProperties(_memid);
	}

	/// @return id of the first memory matchig requirements of the given buffer and Props
	/// If requirements are not matched memory properties defined in Props are relaxed to
	/// those of the fallback.
	/// This may cause for example allocation in host-visible memory when device-local
	/// was originally requested but not available on a given device.
	/// This would only be reported through reporter associated with Instance, and no error
	/// raised.
	static auto findMemory(const vuh::Device& device ///< device on which to search for suitable memory
	                       , vk::Buffer buffer       ///< buffer to find suitable memory for
	                       , vk::MemoryPropertyFlags flags_memory={} ///< additional memory flags
	                       )-> uint32_t 
	{
		auto memid = device.selectMemory(buffer
		                           , vk::MemoryPropertyFlags(vk::MemoryPropertyFlags(Props::memory)
		                             | flags_memory));
		if(memid != uint32_t(-1)){
			return memid;
		}
		device.instance().report("AllocDevice could not find desired memory type, using fallback", " "
		                         , VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
		return AllocFallback::findMemory(device, buffer, flags_memory);
	}
private: // data
	uint32_t _memid = uint32_t(-1); ///< allocated memory id
}; // class AllocDevice

/// Specialize allocator for void properties type.
/// Calls to this class methods basically means all other failed and nothing can be done.
/// This means most its methods throw exceptions.
template<>
class AllocDevice<void>{
public:
	using properties_t = void;
	
	/// @throws vk::OutOfDeviceMemoryError
	auto allocMemory(vuh::Device&, vk::Buffer, vk::MemoryPropertyFlags)-> vk::DeviceMemory {
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(0);
		return vk::DeviceMemory();
#else
		throw vk::OutOfDeviceMemoryError("failed to allocate device memory"
		                                 " and no fallback available");
#endif
	}
	
	/// @throws vuh::NoSuitableMemoryFound
	static auto findMemory(const vuh::Device&, vk::Buffer, vk::MemoryPropertyFlags flags
	                       )-> uint32_t
	{
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(0);
		return VK_NULL_HANDLE;
#else
		throw NoSuitableMemoryFound("no memory with flags " + std::to_string(uint32_t(flags))
		                            + " could be found and not fallback available");
#endif
	}

	/// Create buffer. Normally this should only be called in tests.
	static auto makeBuffer(vuh::Device& device ///< device to create buffer on
	                      , size_t size_bytes  ///< desired size in bytes
	                      , vk::BufferUsageFlags flags ///< additional buffer usage flags
	                      , vk::Result& result
	                      )-> vk::Buffer
	{
		vk::ResultValueType<vk::Buffer>::type buffer = device.createBuffer({ {}, size_bytes, flags});
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		result = buffer.result;
		VULKAN_HPP_ASSERT(vk::Result::eSuccess == result);
		return buffer.value;
#else
		result = vk::Result::eSuccess;
		return buffer;
#endif
	}
	
	/// @throw std::logic_error
	/// Should not normally be called.
	auto memoryProperties(vuh::Device&) const-> vk::MemoryPropertyFlags {
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(0);
		return vk::MemoryPropertyFlags();
#else
		throw std::logic_error("this functions is not supposed to be called");
#endif
	}

	/// @throw std::logic_error
	/// Should not normally be called.
	auto memId() const-> uint32_t {
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(0);
		return VK_NULL_HANDLE;
#else
		throw std::logic_error("this function is not supposed to be called");
#endif
	}
};

} // namespace arr
} // namespace vuh
