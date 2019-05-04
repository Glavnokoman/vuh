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

	auto copy( VkBuffer src, VkBuffer dst, std::size_t size_bytes
	         , std::size_t src_offset=0u, std::size_t dst_offset=0u)-> void;

private:
	VkQueue _handle;
	VkDevice _device;
	VkCommandPool _command_pool;  ///<
	std::uint32_t _family_id;     ///< family id
}; // struct Queue

} // namespace vuh
