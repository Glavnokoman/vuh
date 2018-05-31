#pragma once

#include <vuh/device.h>

#include <vulkan/vulkan.hpp>

#include <cassert>

namespace vuh {
namespace arr {

/// Helper class to allocate memory on a device (incl. host-visible space) 
/// and initialize the buffer.
template<class Props>
class AllocDevice{
public:
	using properties_t = Props;
	using AllocFallback = AllocDevice<typename Props::fallback_t>;

	///
	static auto makeBuffer(vuh::Device& device
	                      , uint32_t size_bytes
	                      , vk::BufferUsageFlags flags ///< additional buffer usage flags
	                      )-> vk::Buffer
	{
		const auto flags_combined = flags | vk::BufferUsageFlags(Props::buffer);
		return device.createBuffer({ {}, size_bytes, flags_combined});
	}

	///
	auto allocMemory(vuh::Device& device, vk::Buffer buffer
	                 , vk::MemoryPropertyFlags flags_memory={}
	                 )-> vk::DeviceMemory 
	{
		_memid = findMemory(device, buffer, flags_memory);
		auto mem = vk::DeviceMemory{};
		try{
			mem = device.allocateMemory({device.getBufferMemoryRequirements(buffer).size, _memid});
		} catch (vk::Error&){
//			std::cerr << "forward the error message here" << "\n";
			auto allocFallback = AllocFallback{};
			mem = allocFallback.allocMemory(device, buffer, flags_memory);
			_memid = allocFallback.memId();
		}
		return mem;
	}

	///
	auto memId() const-> uint32_t {
		assert(_memid != uint32_t(-1)); // should only be called after successful allocMemory() call
		return _memid;
	}

	///
	auto memoryProperties(vuh::Device& device) const-> vk::MemoryPropertyFlags {
		return device.memoryProperties(_memid);
	}

	///
	static auto findMemory(const vuh::Device& device, vk::Buffer buffer
	                       , vk::MemoryPropertyFlags flags_memory={}
	                       )-> uint32_t 
	{
		auto memid = device.selectMemory(buffer
		                           , vk::MemoryPropertyFlags(vk::MemoryPropertyFlags(Props::memory)
		                             | flags_memory));
		if(memid != uint32_t(-1)){
			return memid;
		}
 //		std::cerr << "no memory with desired (" << Props::memory << ") properties found. Trying the fallback"; // TODO: add error reporting
		return AllocFallback::findMemory(device, buffer, flags_memory);
	}
private: // data
	uint32_t _memid = uint32_t(-1); ///< allocated memory id
}; // class AllocDevice

/// Specialize allocator for void properties type.
/// Calls to this class methods basically means all other failed and nothing can be done.
template<>
class AllocDevice<void>{
public:
	using properties_t = void;
	
	///
	auto allocMemory(vuh::Device&, vk::Buffer, vk::MemoryPropertyFlags)-> vk::DeviceMemory {
		throw std::runtime_error("failed to allocate memory for buffer");
	}
	
	///
	static auto findMemory(const vuh::Device&, vk::Buffer, vk::MemoryPropertyFlags)-> uint32_t {
		throw std::runtime_error("no suitable memory found");
	}

	/// Create buffer. Normally this should only be called in tests.
	static auto makeBuffer(vuh::Device& device
	                      , uint32_t size_bytes
	                      , vk::BufferUsageFlags flags ///< additional buffer usage flags
	                      )-> vk::Buffer
	{
		return device.createBuffer({ {}, size_bytes, flags});
	}
	
	auto memoryProperties(vuh::Device&) const-> vk::MemoryPropertyFlags {
		assert(false);
		throw std::runtime_error("this should never happen");
	}
	
	auto memId() const-> uint32_t { 
		assert(false);
		throw std::runtime_error("this should never happen");
	}
};

} // namespace arr
} // namespace vuh
