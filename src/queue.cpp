#include "queue.hpp"
#include "device.hpp"
#include "error.hpp"
#include "defer.hpp"
#include "kernel.hpp"

#include <vulkan/vulkan.h>

namespace vuh {

/// Enques the copy operation between the buffers for execution in the given queue
auto Queue::copy( VkBuffer src
                , VkBuffer dst
                , std::size_t size_bytes
                , std::size_t src_offset
                , std::size_t dst_offset
                )-> Queue&
{
	if(not _pipeline){
		_pipeline = std::make_unique<Pipeline>(*this);
	}

	const auto cmdbuf_info = VkCommandBufferAllocateInfo {
	                                       VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr
	                                       , _command_pool
	                                       , VK_COMMAND_BUFFER_LEVEL_PRIMARY
	                                       , 1 };
	auto cmdbuf = VkCommandBuffer{};
	VUH_CHECK_RET(vkAllocateCommandBuffers(_device, &cmdbuf_info, &cmdbuf), *this);

	const auto begin_info = VkCommandBufferBeginInfo{
	                                      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr
	                                      , {} // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT ??
	                                      , nullptr};
	VUH_CHECK_RET(vkBeginCommandBuffer(cmdbuf, &begin_info), *this);
	auto region = VkBufferCopy{ src_offset, dst_offset, size_bytes };
	vkCmdCopyBuffer(cmdbuf, src, dst, 1, &region);
	VUH_CHECK_RET(vkEndCommandBuffer(cmdbuf), *this);

	_pipeline->push_data(SubmitData{
	                    _pipeline->semaphores_outstanding
	                    , std::vector<VkPipelineStageFlags>(_pipeline->semaphores_outstanding.size()
	                                                       , VK_PIPELINE_STAGE_TRANSFER_BIT)
	                    , cmdbuf // cmdbuf for transfer operations owned by the Pipeline object
	                    });
	_pipeline->semaphores_outstanding.clear();
	return *this;
}

/// Enques the fully bound kernel for execution in the given queue.
auto Queue::run(Kernel& k)-> Queue& {
	if(not _pipeline){
		_pipeline = std::make_unique<Pipeline>(*this);
	}

	_pipeline->push_data(SubmitData{
	                   _pipeline->semaphores_outstanding
	                 , std::vector<VkPipelineStageFlags>( _pipeline->semaphores_outstanding.size()
	                                                    , VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
	                 , nullptr /*cmdbuf is owned by the compute kernel*/
	                 }
	                 , k.command_buffer(_command_pool));
	_pipeline->semaphores_outstanding.clear();
	return *this;
}

/// Detach the currently constructed pipeline from the queue.
auto Queue::release_pipeline()-> std::unique_ptr<Pipeline> {
	return std::move(_pipeline);
}

/// Blocks till the queue becomes idle.
auto Queue::waitIdle() const-> void { VUH_CHECK(vkQueueWaitIdle(_handle)); }

/// Submits the currently constructed pipeline for execution and flushes the queue state. Nonblocking.
/// @return Host synchronization token.
auto Queue::hb()-> SyncTokenHost {
	_pipeline->set_fence();
	VUH_CHECKOUT_RET(SyncTokenHost(nullptr));

	_pipeline->submit();
	VUH_CHECKOUT_RET(SyncTokenHost(nullptr));
	return SyncTokenHost(std::move(_pipeline));
}

/// Submits the currently constructed pipeline for executaion and flushes the queue state.
/// Blocks till the queue becomes idle.
auto Queue::submit()-> void {
	_pipeline->submit();
	const auto err = vkQueueWaitIdle(_handle);
	_pipeline = nullptr;
	VUH_CHECK(err);
}

///
auto Pipeline::submit()-> void {
	VUH_CHECK(vkQueueSubmit(queue, submit_infos.size(), submit_infos.data(), fence));
}

///
auto Pipeline::set_fence()-> void {
	const auto fence_info = VkFenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, {}};
	VUH_CHECK(vkCreateFence(queue.device(), &fence_info, nullptr, &fence));
}

/// @throws
Pipeline::~Pipeline() {
	auto err = VK_SUCCESS;
	if(fence) {
		err = vkWaitForFences(queue.device(), 1, &fence, VK_TRUE, uint64_t(-1));
		vkDestroyFence(queue.device(), fence, nullptr);
		fence = nullptr;
	}
	for(auto& s: semaphores_owned){
		vkDestroySemaphore(queue.device(), s, nullptr);
	}
	VUH_CHECK(err);
}

///
auto Pipeline::push_data(SubmitData data)-> void {
	const auto& submitted = submit_data.emplace_back(std::move(data));
	auto submit_info = VkSubmitInfo{
	                   VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr
	                   , uint32_t(submitted.wait_for.size())
	                   , submitted.wait_for.data()
	                   , submitted.stage_flags.data()
	                   , 1, &submitted.cmd_buf
	                   , 0, nullptr // signalling semaphores, not defined at this stage
	};
	submit_infos.push_back(submit_info);
}

///
auto Pipeline::push_data(SubmitData data, const VkCommandBuffer& buf)-> void {
	assert(data.cmd_buf == nullptr);
	const auto& submitted = submit_data.emplace_back(std::move(data));
	auto submit_info = VkSubmitInfo{
	                   VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr
	                   , uint32_t(submitted.wait_for.size())
	                   , submitted.wait_for.data()
	                   , submitted.stage_flags.data()
	                   , 1, &buf
	                   , 0, nullptr // signalling semaphores, not defined at this stage
	};
	submit_infos.push_back(submit_info);
}

/// Run the fully specified kernel in the default compute queue of kernel device synchronously.
/// Wait for completion.
auto run(Kernel& k)-> void {
	auto& que = k.device().default_compute();
	que.run(k);
	que.submit();
}

///
auto run_async(Kernel& k)-> SyncTokenHost {
	auto& que = k.device().default_compute();
	que.run(k);
	return que.hb();
}
} // namespace vuh
