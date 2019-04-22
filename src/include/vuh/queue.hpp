#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>

namespace vuh {

/// Queue in a context of a compute device
class Queue {
public:
	explicit Queue(VkQueue handle, VkCommandPool& command_pool, std::uint32_t family_id)
	   : _handle(handle), _command_pool(command_pool), _family_id(family_id)
	{}

private:
	VkQueue _handle;
	VkCommandPool& _command_pool; ///<
	std::uint32_t _family_id;     ///< family id
}; // struct Queue

} // namespace vuh
