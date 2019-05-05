#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace vuh {

/// Queue in a context of a compute device
class Queue {
public:
	explicit Queue( VkQueue handle
	              , VkDevice device
	              , VkCommandPool command_pool
	              , std::uint32_t family_id
	              )
	   : _handle(handle), _device(device), _command_pool(command_pool), _family_id(family_id)
	{}

	auto copy_sync( VkBuffer src, VkBuffer dst, std::size_t size_bytes
	              , std::size_t src_offset=0u, std::size_t dst_offset=0u)-> void;

	template<class BufIn, class BufOut>
	auto copy_sync(const BufIn& src, BufOut& dst){
		copy_sync(src.buffer(), dst.buffer(), src.size_bytes(), src.offset_bytes(), dst.offset_bytes());
	}

private:
	VkQueue _handle;
	VkDevice _device;
	VkCommandPool _command_pool;  ///<
	std::uint32_t _family_id;     ///< family id
}; // struct Queue

} // namespace vuh
