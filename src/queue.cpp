#include "queue.hpp"
#include "error.hpp"
#include "defer.hpp"

namespace vuh {

/// Synchronous copy chunk of memory between VkBuffer-s
auto Queue::copy( VkBuffer src
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
	auto err = vkAllocateCommandBuffers(_device, &allocate_info, &cmd_buf);
	VUH_CHECK(err);
	auto buf_guard = utils::defer([&]{vkFreeCommandBuffers(_device, _command_pool, 1, &cmd_buf);});

	auto begin_info = VkCommandBufferBeginInfo{}; // hierwasik
	err = vkBeginCommandBuffer(cmd_buf, &begin_info);

//	auto cmd_buf = device.transferCmdBuffer();
//	cmd_buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
//	auto region = vk::BufferCopy(src_offset, dst_offset, size_bytes);
//	cmd_buf.copyBuffer(src, dst, 1, &region);
//	cmd_buf.end();
//	auto queue = device.transferQueue();
//	auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buf);
//	queue.submit({submit_info}, nullptr);
//	queue.waitIdle();
}


} // namespace vuh

