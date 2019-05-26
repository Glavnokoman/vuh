#include "queue.hpp"
#include "device.hpp"
#include "error.hpp"
#include "defer.hpp"
#include "kernel.hpp"

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
	vkFreeCommandBuffers(_device, _command_pool, 1, &cmd_buf);
}

/// Enques the fully bound kernel for execution in the given queue.
auto Queue::run(Kernel& k)-> Queue& {
	if(not _submission){
		_submission = std::make_unique<Pipeline>(*this);
	}
	_submission->submit_data.emplace_back(SubmitData{
	                   _submission->semaphores_outstanding
	                 , std::vector<VkPipelineStageFlags>( _submission->semaphores_outstanding.size()
	                                                    , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
	                 , nullptr /*cmdbuf is owned by the compute kernel*/
	                 });
	const auto& cmdbuf = k.command_buffer(_command_pool);
	auto submit_info = VkSubmitInfo{
	                   VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr
	                   , uint32_t(_submission->submit_data.back().wait_for.size())
	                   , _submission->submit_data.back().wait_for.data()
	                   , _submission->submit_data.back().stage_flags.data()
	                   , 1, &cmdbuf
	                   , 0, nullptr // signalling semaphores, not defined at this stage
	};
	_submission->submit_infos.push_back(submit_info); // @bug: all pointers in the submit_info will be invalid by the time it is actually used.
	_submission->semaphores_outstanding.clear();

	return *this;
}

///
auto Pipeline::submit()-> void {
	VUH_CHECK(vkQueueSubmit(queue, submit_infos.size(), submit_infos.data(), fence));
}

///
Pipeline::~Pipeline() noexcept {
	if(fence) {
		vkDestroyFence(queue.device(), fence, nullptr);
	}
}


///
auto Queue::submit()-> void { _submission->submit(); }

///
auto Queue::waitIdle() const-> void { VUH_CHECK(vkQueueWaitIdle(_handle)); }

/// Run the fully specified kernel in the default compute queue of kernel device synchronously.
/// Wait for completion.
auto run(Kernel& k)-> void {
	auto& que = k.device().default_compute();
	que.run(k);
	que.submit();
	que.waitIdle();
}

} // namespace vuh
