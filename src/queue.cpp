#include "queue.hpp"
#include "error.hpp"
#include "defer.hpp"

#include <vulkan/vulkan.h>

namespace vuh {

/// Synchronous copy chunk of memory between VkBuffer-s
auto Queue::copy_sync( VkBuffer src
                , VkBuffer dst
                , std::size_t size_bytes
                , std::size_t src_offset
                , std::size_t dst_offset
                )-> void
{
	auto allocate_info = VkCommandBufferAllocateInfo {
	                     VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr
	                     , _command_pool
	                     , VK_COMMAND_BUFFER_LEVEL_PRIMARY
	                     , 1 };
	auto cmd_buf = VkCommandBuffer{};
	VUH_CHECK(vkAllocateCommandBuffers(_device, &allocate_info, &cmd_buf));
	auto buf_guard = utils::defer([&]{vkFreeCommandBuffers(_device, _command_pool, 1, &cmd_buf);});

	auto begin_info = VkCommandBufferBeginInfo{};
	VUH_CHECK(vkBeginCommandBuffer(cmd_buf, &begin_info));
	auto region = VkBufferCopy{ src_offset, dst_offset, size_bytes };
	vkCmdCopyBuffer(cmd_buf, src, dst, 1, &region);
	VUH_CHECK(vkEndCommandBuffer(cmd_buf));
	auto submit_info = VkSubmitInfo{
	                   VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr
	                   , 0, nullptr, nullptr
	                   , 1, &cmd_buf
	                   , 0, nullptr };
	VUH_CHECK(vkQueueSubmit(_handle, 1, &submit_info, nullptr));
	VUH_CHECK(vkQueueWaitIdle(_handle));
}


} // namespace vuh

