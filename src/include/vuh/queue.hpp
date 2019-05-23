#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace vuh {

///
class Submission{
public:

//	auto run()-> Submission;
//	auto cb()-> Submission; // set compute barrier
// auto tb()-> Submission; // set transfer barrier

private:
	VkDevice _device; ///< non-owning
	VkQueue  _queue;  ///< non-owning
	std::vector<VkSubmitInfo> _submits;
}; // class Submission

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

	///
	auto run()-> Submission {

	}
private:
	VkQueue _handle;              ///< owned queue handle
	VkDevice _device;             ///< non-owned logical device handle containing the queue and other stuff
	VkCommandPool _command_pool;  ///< non-owned command pool handle. command pool should correspond to the queue family to which the queue handle belongs
	std::uint32_t _family_id;     ///< queue family id. Needed here?
}; // struct Queue

} // namespace vuh
