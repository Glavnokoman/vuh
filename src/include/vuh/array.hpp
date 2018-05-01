#pragma once

#include "device.h"
#include "hostVisibleMemView.hpp"

#include <vulkan/vulkan.hpp>

#include <cassert>

namespace vuh {
	using std::begin;
	using std::end;

	namespace detail {
		/// Crutch to modify buffer usage flags to include transfer bit in case those may be needed.
		inline auto update_usage(const vuh::Device& device
		                        , vk::MemoryPropertyFlags properties
		                        , vk::BufferUsageFlags usage
		                        )-> vk::BufferUsageFlags
		{
			if(device.properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu
			   && properties == vk::MemoryPropertyFlagBits::eDeviceLocal
			   && usage == vk::BufferUsageFlagBits::eStorageBuffer)
			{
				usage |= vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst;
			}
			return usage;
		}

		/// Copy device buffers using the transient command pool.
		/// Fully sync, no latency hiding whatsoever.
		inline auto copyBuf(vuh::Device& device
		                    , vk::Buffer src, vk::Buffer dst
		                    , uint32_t size  ///< size of memory chunk to copy in bytes
		                    )-> void
		{
			auto cmd_buf = device.transferCmdBuffer();
			cmd_buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
			auto region = vk::BufferCopy(0, 0, size);
			cmd_buf.copyBuffer(src, dst, 1, &region);
			cmd_buf.end();
			auto queue = device.transferQueue();
			auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buf);
			queue.submit({submit_info}, nullptr);
			queue.waitIdle();
		}
	} // namespace detail

/// Device buffer owning its chunk of memory.
template<class T> 
class Array {
public:
	using value_type = T;
	
	Array(vuh::Device& device, uint32_t n_elements
	      , vk::MemoryPropertyFlags properties=vk::MemoryPropertyFlagBits::eDeviceLocal
	      , vk::BufferUsageFlags usage=vk::BufferUsageFlagBits::eStorageBuffer
	      )
	   : Array(device
	           , device.makeBuffer(n_elements*sizeof(T)
	                               , detail::update_usage(device, properties, usage))
	           , properties, n_elements)
   {}

	~Array() noexcept {release();}
   
   Array(const Array&) = delete;
   Array& operator= (const Array&) = delete;
	/// move constructor. passes the underlying buffer ownership
	Array(Array&& other) noexcept
	   : _buf(other._buf), _mem(other._mem), _flags(other._flags), _dev(other._dev), _size(other._size)
	{
		other._buf = nullptr;
	}

	/// Move assignment. Self move-assignment is UB (as it should be).
	auto operator= (Array&& other) noexcept-> Array& {
		release();
		std::memcpy(this, &other, sizeof(Array));
		other._buf = nullptr;
		return *this;
	}
   
	auto size() const-> uint32_t {return _size;}

   template<class C>
   static auto fromHost(vuh::Device& device, const C& c
	                     , vk::MemoryPropertyFlags properties=vk::MemoryPropertyFlagBits::eDeviceLocal
								, vk::BufferUsageFlags usage=vk::BufferUsageFlagBits::eStorageBuffer
	                     )-> Array
	{
		auto r = Array<T>(device, uint32_t(c.size()), properties, usage);
		if(r._flags & vk::MemoryPropertyFlagBits::eHostVisible){ // memory is host-visible
			std::copy(begin(c), end(c), r.view_host_visible().begin());
		} else { // memory is not host visible, use staging buffer
			auto stage_buf = fromHost(device, c
			                          , vk::MemoryPropertyFlagBits::eHostVisible
			                          , vk::BufferUsageFlagBits::eTransferSrc);
			detail::copyBuf(device, stage_buf, r, stage_buf.size()*sizeof(T));
		}
		return r;
	}
   
	/// copy from device buffer to some host iterable (+resizable()).
	template<class C>
	auto to_host(C& c)-> void {
		if(_flags & vk::MemoryPropertyFlagBits::eHostVisible){ // memory IS host visible
			auto hv = view_host_visible();
			c.resize(size());
			std::copy(begin(hv), end(hv), begin(c));
		} else { // memory is not host visible, use staging buffer
			// copy device memory to staging buffer
			auto stage_buf = Array(_dev, size()
			                      , vk::MemoryPropertyFlagBits::eHostVisible
			                      , vk::BufferUsageFlagBits::eTransferDst);
			detail::copyBuf(_dev, _buf, stage_buf, size()*sizeof(T));
			stage_buf.to_host(c); // copy from staging buffer to host
		}
	}

   template<class C>
   auto toHost(C& c) const-> void {throw "not implemented";}

	operator vk::Buffer& () {return _buf;}
	operator const vk::Buffer& () const {return _buf;}

private: // helpers
	/// Helper constructor
	Array(vuh::Device& device
	     , vk::Buffer buffer
	     , vk::MemoryPropertyFlags properties
	     , uint32_t n_elements
	     )
	   : Array(device, buffer, n_elements, device.selectMemory(buffer, properties))
	{}

	/// Helper constructor. This one does the actual construction and binding.
	Array(Device& device, vk::Buffer buf, uint32_t n_elements, uint32_t mem_id)
	   : _buf(buf)
	   , _mem(device.alloc(buf, mem_id))
	   , _flags(device.memoryProperties(mem_id))
	   , _dev(device)
	   , _size(n_elements)
	{
		device.bindBufferMemory(buf, _mem, 0);
	}
   
	/// release resources associated with current Array object
	auto release() noexcept-> void {
		if(_buf){
			_dev.freeMemory(_mem);
			_dev.destroyBuffer(_buf);
		}
	}

	/// @return array view for host visible device buffer
	auto view_host_visible()-> HostVisibleMemView<T> {
		assert(_flags & vk::MemoryPropertyFlagBits::eHostVisible);
		return HostVisibleMemView<T>(_dev, _mem, _size);
	}
private: // data
	vk::Buffer _buf;                        ///< device buffer
	vk::DeviceMemory _mem;                  ///< associated chunk of device memory
	vk::MemoryPropertyFlags _flags;         ///< Actual flags of allocated memory. Can be a superset of requested flags.
	vuh::Device& _dev;                      ///< referes underlying logical device
	uint32_t _size;                         ///< number of elements. Actual allocated memory may be a bit bigger than necessary.
}; // class Array
} // namespace vuh
