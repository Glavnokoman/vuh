#pragma once

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <vector>

namespace vuh {
class Kernel;
class Queue;

auto run(Kernel& k)-> void;

///
struct SubmitData {
	std::vector<VkSemaphore> wait_for; ///< non-owning
	std::vector<VkPipelineStageFlags> stage_flags;
	VkCommandBuffer cmd_buf; ///< Owning handle to comamnd buffer. Only command buffers owned by submit data (transfer operations) are recorded here. The buffers owned by external objects are recorded directly to submit info.
}; // struct SubmitData

/// doc me. this one is kinda important.
struct Pipeline {
	explicit Pipeline(Queue& queue): queue{queue}{}
	auto submit()-> void;
	auto set_fence()-> void;
	auto push_data(SubmitData data)-> void;
	auto push_data(SubmitData data, const VkCommandBuffer& buf)-> void;

	~Pipeline();
	// nocopy - move only

	Queue& queue; ///< non-owning handle to the queue used to create the pipeline.
	VkFence fence = nullptr; ///< owning handle to a fence for host synchronization. can be null.
	std::vector<VkSubmitInfo> submit_infos;
	std::list<SubmitData>     submit_data;            ///< holds the data which pointed to by submit infos structures
	std::vector<VkSemaphore>  semaphores_owned;       ///< semaphores owned by the pipeline. For some semaphores ownership could have been transferred to outside objects, those should not be contained here.
	std::vector<VkSemaphore>  semaphores_outstanding; ///< signifies the set of wait semaphores for the next submission. @todo: move it to the Queue where it actually belongs.
}; // class Submission

///
class SyncTokenHost {
public:
	explicit SyncTokenHost(std::unique_ptr<Pipeline> pipeline): _pipeline(std::move(pipeline)) {}
	auto wait()-> void { _pipeline = nullptr; }
private:
	std::unique_ptr<Pipeline> _pipeline;
}; // struct SyncTokenHost

/// Host synchronization token carrying an additional resource on itself.
/// For instance a staging buffer for async copy.
template<class T>
class SyncTokenHostResource: private std::unique_ptr<T>, public SyncTokenHost {
public:
	SyncTokenHostResource(SyncTokenHost&& token, T&& resource)
	   : std::unique_ptr<T>(std::make_unique<T>(std::move(resource)))
	   , SyncTokenHost(std::move(token))
	{}
};

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

//	auto copy_sync( VkBuffer src, VkBuffer dst, std::size_t size_bytes
//	              , std::size_t src_offset=0u, std::size_t dst_offset=0u)-> void;

	auto copy( VkBuffer src, VkBuffer dst, std::size_t size_bytes
	         , std::size_t src_offset=0u, std::size_t dst_offset=0u)-> Queue&;

	template<class BufIn, class BufOut>
	auto copy(const BufIn& src, BufOut& dst)-> Queue& {
		return copy( src.buffer(), dst.buffer()
		           , src.size_bytes(), src.offset_bytes(), dst.offset_bytes());
	}

	///
	auto run(Kernel&)-> Queue&;
//	auto qb()-> SyncTokenQueue;
	auto hb()-> SyncTokenHost;
	template<class T> auto hb(T&& resource)-> SyncTokenHostResource<T>{
		return SyncTokenHostResource<T>{hb(), std::move(resource)};
	}
	auto release_pipeline()-> std::unique_ptr<Pipeline>;

	operator VkQueue() const { return _handle; }
	auto device() const-> VkDevice { return _device; }

	auto submit()-> void;
	auto waitIdle() const-> void;
private:
	VkQueue _handle;              ///< owned queue handle
	VkDevice _device;             ///< non-owned logical device handle containing the queue and other stuff
	VkCommandPool _command_pool;  ///< non-owned command pool handle. command pool should correspond to the queue family to which the queue handle belongs
	std::unique_ptr<Pipeline> _pipeline; ///< computation pipeline assembled for submission in current queue
	std::uint32_t _family_id;     ///< queue family id. Needed here?
}; // struct Queue

} // namespace vuh
