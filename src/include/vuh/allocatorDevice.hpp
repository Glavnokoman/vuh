#pragma once

#include "device.hpp"
#include "error.hpp"
#include "instance.hpp"
#include "utils.hpp"

#include <vulkan/vulkan_core.h>

#include <cassert>

namespace vuh {

struct DeviceMemory {
	VkDeviceMemory memory; ///< doc me
	std::uint32_t id;      ///< memory id on a physical device
};

/// Helper class to allocate memory directly from a device memory (incl. host-visible space)
/// and initialize the buffer.
/// Binding between memory and buffer is to be done elsewhere.
template<class Traits>
class AllocatorDevice {
public:
	using properties_t = Traits; // @todo: rename
	using AllocFallback = AllocatorDevice<typename Traits::fallback_t>; ///< fallback allocator

	/// Create buffer on a device.
	static auto makeBuffer( vuh::Device& device      ///< device to create buffer on
	                      , std::size_t size_bytes   ///< desired size in bytes
	                      , VkBufferUsageFlags flags ///< additional (to the ones defined in Props) buffer usage flags
	                      )-> VkBuffer
	{
		auto ret = VkBuffer{};
		const auto create_info = VkBufferCreateInfo{
		                             VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr // pNext
		                             , VkBufferCreateFlags{}
		                             , size_bytes
		                             , flags | Traits::buffer
		                             , VK_SHARING_MODE_EXCLUSIVE
		                             , 0, nullptr };
		auto err = vkCreateBuffer(device, &create_info, nullptr, &ret);
		VUH_CHECK_RET(err, ret);
		return ret;
	}

	/// Allocate memory for the buffer.
	static auto allocMemory( vuh::Device& device  ///< device to allocate memory
	                       , VkBuffer buffer      ///< buffer to allocate memory for
	                       , VkMemoryPropertyFlags flags_memory={} ///< additional (to the ones defined in Props) memory property flags
	                       )-> DeviceMemory
	{
		auto memory_requirements = VkMemoryRequirements{};
		vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
		for(const auto mem_id: findMemory(device, memory_requirements, flags_memory)){
			const auto alloc_info = VkMemoryAllocateInfo{
			                            VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO , nullptr // pNext
			                            , memory_requirements.size
			                            , mem_id};
			auto mem = VkDeviceMemory{};
			auto err = vkAllocateMemory(device, &alloc_info, nullptr, &mem);
			if(err == VK_SUCCESS){
				return {mem, mem_id};
			}
		}
		auto allocFallback = AllocFallback{};
		device.instance().log("AllocatorDevice failed to allocate memory. Using the fallback"
		                     , error::text(err)
		                     , VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
		return allocFallback.allocMemory(device, buffer, flags_memory);
	}

	/// @return ids of the memories matchig requirements and additional property flags.
	/// If requirements are not matched memory Traits requirements are relaxed to
	/// those of the fallback.
	/// This may cause for example allocation in the host-visible memory when device-local
	/// was originally requested but not available on a given device.
	/// In such scenario the log message will be issued but no error raised.
	static auto findMemory( const vuh::Device& device  ///< device to search for suitable memory
	                      , VkMemoryRequirements requirements     ///< buffer to find suitable memory for
	                      , VkMemoryPropertyFlags flags_memory={} ///< additional memory flags
	                      )-> std::vector<std::uint32_t>
	{
		auto ret = selectMemory(device.physical(), requirements, Traits::memory | flags_memory);
		if(!ret.empty()){
			return ret;
		}
		device.instance().log("AllocatorDevice could not find desired memory type. Using fallback"
		                     , " "
		                     , VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
		return AllocFallback::findMemory(device, requirements, flags_memory);
	}
}; // class AllocatorDevice

/// Specialize allocator for void properties type.
/// Calls to this class methods basically means all other failed and nothing can be done.
/// This means most its methods throw exceptions.
template<>
class AllocatorDevice<void>{
public:
	using properties_t = void;
	
	/// @throws vk::OutOfDeviceMemoryError
	static auto allocMemory(vuh::Device&, VkBuffer, VkMemoryPropertyFlags)-> DeviceMemory {
		VUH_SIGNAL(VK_ERROR_OUT_OF_DEVICE_MEMORY);
	}
	
	/// @throws vuh::NoSuitableMemoryFound
	static auto findMemory(const vuh::Device& device, VkMemoryRequirements
	                       , VkMemoryPropertyFlags flags
	                       )-> std::vector<std::uint32_t>
	{
		device.instance().log("no memory with flags " + std::to_string(uint32_t(flags))
		                      + " could be found and not fallback available"
		                      , "" , VK_DEBUG_REPORT_ERROR_BIT_EXT);
		VUH_SIGNAL(error::NoSuitableMemoryFound);
	}

	/// Create buffer. Normally this should only get called in tests.
	static auto makeBuffer( vuh::Device& device      ///< device to create buffer on
	                      , std::size_t size_bytes   ///< desired size in bytes
	                      , VkBufferUsageFlags flags ///< additional buffer usage flags
	                      )-> VkBuffer
	{
		auto ret = VkBuffer{};
		const auto create_info = VkBufferCreateInfo{
		                             VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, nullptr // pNext
		                             , VkBufferCreateFlags{}
		                             , size_bytes
		                             , flags
		                             , VK_SHARING_MODE_EXCLUSIVE
		                             , 0, nullptr };
		auto err = vkCreateBuffer(device, &create_info, nullptr, &ret);
		VUH_CHECK_RET(err, ret);
		return ret;
	}
};

} // namespace vuh
