#pragma once

#include <vuh/device.h>
#include <vuh/error.h>
#include <vuh/instance.h>
#include <vulkan/vulkan.hpp>
#include <cassert>

namespace vuh {
namespace mem {

/// Helper class to allocate memory directly from a device memory (incl. host-visible space)
/// (as opposed to allocating from a pool) and initialize the buffer.
/// Binding between memory and buffer is done elsewhere.
template<class Props>
class AllocDevice {
public:
	using properties_t = Props;
	using AllocFallback = AllocDevice<typename Props::fallback_t>; ///< fallback allocator

	/// Create buffer on a device.
	static auto makeBuffer(vuh::Device& device   ///< device to create buffer on
	                      , size_t size_bytes    ///< desired size in bytes
	                      , VULKAN_HPP_NAMESPACE::BufferUsageFlags flags ///< additional (to the ones defined in Props) buffer usage flags
	                      , VULKAN_HPP_NAMESPACE::Result& result
	                      )-> VULKAN_HPP_NAMESPACE::Buffer
	{
		const auto flags_combined = flags | VULKAN_HPP_NAMESPACE::BufferUsageFlags(Props::buffer);

		auto buffer = device.createBuffer({ {}, size_bytes, flags_combined});
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		result = buffer.result;
		VULKAN_HPP_ASSERT(VULKAN_HPP_NAMESPACE::Result::eSuccess == result);
		return buffer.value;
#else
		result = VULKAN_HPP_NAMESPACE::Result::eSuccess;
		return buffer;
#endif
	}

	/// Create image on a device.
	static auto makeImage(vuh::Device& device   ///< device to create buffer on
            , VULKAN_HPP_NAMESPACE::ImageType imageType
            , size_t width    ///< desired width
            , size_t height    ///< desired height
            , VULKAN_HPP_NAMESPACE::Format format /// format
            , VULKAN_HPP_NAMESPACE::ImageUsageFlags flags ///< additional (to the ones defined in Props) image usage flags
			, VULKAN_HPP_NAMESPACE::Result& result
	)-> VULKAN_HPP_NAMESPACE::Image
	{
        const auto flags_combined = flags | VULKAN_HPP_NAMESPACE::ImageUsageFlags(Props::image);

        VULKAN_HPP_NAMESPACE::Extent3D extent;
        extent.setWidth(width);
        extent.setHeight(height);
        extent.setDepth(1);

        VULKAN_HPP_NAMESPACE::ImageCreateInfo imageCreateInfo;
        imageCreateInfo.setImageType(imageType);
        imageCreateInfo.setExtent(extent);
        imageCreateInfo.setUsage(flags_combined);
        imageCreateInfo.setTiling(VULKAN_HPP_NAMESPACE::ImageTiling::eOptimal);

		auto image = device.createImage(imageCreateInfo);
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		result = image.result;
		VULKAN_HPP_ASSERT(VULKAN_HPP_NAMESPACE::Result::eSuccess == result);
		return image.value;
#else
		result = VULKAN_HPP_NAMESPACE::Result::eSuccess;
		return image;
#endif
	}

	/// Allocate memory for the image.
	auto allocMemory(vuh::Device& device  ///< device to allocate memory
			, VULKAN_HPP_NAMESPACE::Image image  ///< buffer to allocate memory for
			, VULKAN_HPP_NAMESPACE::MemoryPropertyFlags flags_memory={} ///< additional (to the ones defined in Props) memory property flags
	)-> VULKAN_HPP_NAMESPACE::DeviceMemory
	{
		_memid = findMemory(device, image, flags_memory);
		auto mem = VULKAN_HPP_NAMESPACE::DeviceMemory{};
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		auto alloc_mem = device.allocateMemory({device.getImageMemoryRequirements(image).size, _memid});
		VULKAN_HPP_ASSERT(VULKAN_HPP_NAMESPACE::Result::eSuccess == alloc_mem.result);
		if (VULKAN_HPP_NAMESPACE::Result::eSuccess == alloc_mem.result) {
			mem = alloc_mem.value;
		} else {
			device.instance().report("AllocDevice failed to allocate memory, using fallback", VULKAN_HPP_NAMESPACE::to_string(alloc_mem.result).c_str()
					, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
#else
		try{
			mem = device.allocateMemory({device.getImageMemoryRequirements(image).size, _memid});
		} catch (VULKAN_HPP_NAMESPACE::Error& e){
			device.instance().report("AllocDevice failed to allocate memory, using fallback", e.what()
			                         , VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
#endif
			auto allocFallback = AllocFallback{};
			mem = allocFallback.allocMemory(device, image, flags_memory);
			_memid = allocFallback.memId();
		}

		return mem;
	}

	/// Allocate memory for the buffer.
	auto allocMemory(vuh::Device& device  ///< device to allocate memory
	                 , VULKAN_HPP_NAMESPACE::Buffer buffer  ///< buffer to allocate memory for
	                 , VULKAN_HPP_NAMESPACE::MemoryPropertyFlags flags_memory={} ///< additional (to the ones defined in Props) memory property flags
	                 )-> VULKAN_HPP_NAMESPACE::DeviceMemory
	{
		_memid = findMemory(device, buffer, flags_memory);
		auto mem = VULKAN_HPP_NAMESPACE::DeviceMemory{};
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		auto alloc_mem = device.allocateMemory({device.getBufferMemoryRequirements(buffer).size, _memid});
		VULKAN_HPP_ASSERT(VULKAN_HPP_NAMESPACE::Result::eSuccess == alloc_mem.result);
		if (VULKAN_HPP_NAMESPACE::Result::eSuccess == alloc_mem.result) {
			mem = alloc_mem.value;
		} else {
			device.instance().report("AllocDevice failed to allocate memory, using fallback", VULKAN_HPP_NAMESPACE::to_string(alloc_mem.result).c_str()
					, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
#else
		try{
			mem = device.allocateMemory({device.getBufferMemoryRequirements(buffer).size, _memid});
		} catch (VULKAN_HPP_NAMESPACE::Error& e){
			device.instance().report("AllocDevice failed to allocate memory, using fallback", e.what()
			                         , VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
#endif
			auto allocFallback = AllocFallback{};
			mem = allocFallback.allocMemory(device, buffer, flags_memory);
			_memid = allocFallback.memId();
		}

		return mem;
	}

	/// @return memory id on which actual allocation took place.
	auto memId() const-> uint32_t {
		assert(_memid != uint32_t(-1)); // should only be called after successful allocMemory() call
		return _memid;
	}

	/// @return memory property flags of the memory on which actual allocation took place.
	auto memoryProperties(const vuh::Device& device) const-> VULKAN_HPP_NAMESPACE::MemoryPropertyFlags {
		return device.memoryProperties(_memid);
	}

	/// @return id of the first memory matched requirements of the given buffer and Props
	/// If requirements are not matched memory properties defined in Props are relaxed to
	/// those of the fallback.
	/// This may cause for example allocation in host-visible memory when device-local
	/// was originally requested but not available on a given device.
	/// This would only be reported through reporter associated with Instance, and no error
	/// raised.
	static auto findMemory(vuh::Device& device ///< device on which to search for suitable memory
	                       , VULKAN_HPP_NAMESPACE::Buffer& buffer       ///< buffer to find suitable memory for
	                       , VULKAN_HPP_NAMESPACE::MemoryPropertyFlags flags_memory={} ///< additional memory flags
	                       )-> uint32_t 
	{
		auto memid = device.selectMemory(buffer
		                           , VULKAN_HPP_NAMESPACE::MemoryPropertyFlags(VULKAN_HPP_NAMESPACE::MemoryPropertyFlags(Props::memory)
		                             | flags_memory));
		if(memid != uint32_t(-1)){
			return memid;
		}
		device.instance().report("AllocDevice could not find desired memory type, using fallback", " "
		                         , VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
		return AllocFallback::findMemory(device, buffer, flags_memory);
	}

	/// @return id of the first memory match requirements of the given image and Props
	/// If requirements are not matched memory properties defined in Props are relaxed to
	/// those of the fallback.
	/// This may cause for example allocation in host-visible memory when device-local
	/// was originally requested but not available on a given device.
	/// This would only be reported through reporter associated with Instance, and no error
	/// raised.
	static auto findMemory(vuh::Device& device ///< device on which to search for suitable memory
			, VULKAN_HPP_NAMESPACE::Image& image       ///< image to find suitable memory for
			, VULKAN_HPP_NAMESPACE::MemoryPropertyFlags flags_memory={} ///< additional memory flags
	)-> uint32_t
	{
		auto memid = device.selectMemory(image
				, VULKAN_HPP_NAMESPACE::MemoryPropertyFlags(VULKAN_HPP_NAMESPACE::MemoryPropertyFlags(Props::memory)
															| flags_memory));
		if(memid != uint32_t(-1)){
			return memid;
		}
		device.instance().report("AllocDevice could not find desired memory type, using fallback", " "
				, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
		return AllocFallback::findMemory(device, image, flags_memory);
	}
private: // data
	uint32_t _memid = uint32_t(-1); ///< allocated memory id
}; // class AllocDevice

/// Specialize allocator for void properties type.
/// Calls to this class methods basically means all other failed and nothing can be done.
/// This means most its methods throw exceptions.
template<>
class AllocDevice<void>{
public:
	using properties_t = void;
	
	/// @throws VULKAN_HPP_NAMESPACE::OutOfDeviceMemoryError
	auto allocMemory(vuh::Device&, VULKAN_HPP_NAMESPACE::Buffer&, VULKAN_HPP_NAMESPACE::MemoryPropertyFlags)-> VULKAN_HPP_NAMESPACE::DeviceMemory {
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(0);
		return VULKAN_HPP_NAMESPACE::DeviceMemory();
#else
		throw VULKAN_HPP_NAMESPACE::OutOfDeviceMemoryError("failed to allocate device memory"
		                                 " and no fallback available");
#endif
	}

	/// @throws VULKAN_HPP_NAMESPACE::OutOfDeviceMemoryError
	auto allocMemory(vuh::Device&, VULKAN_HPP_NAMESPACE::Image&, VULKAN_HPP_NAMESPACE::MemoryPropertyFlags)-> VULKAN_HPP_NAMESPACE::DeviceMemory {
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(0);
		return VULKAN_HPP_NAMESPACE::DeviceMemory();
#else
		throw VULKAN_HPP_NAMESPACE::OutOfDeviceMemoryError("failed to allocate device memory"
		                                 " and no fallback available");
#endif
	}
	
	/// @throws vuh::NoSuitableMemoryFound
	static auto findMemory(vuh::Device&, VULKAN_HPP_NAMESPACE::Buffer&, VULKAN_HPP_NAMESPACE::MemoryPropertyFlags flags
	                       )-> uint32_t
	{
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(0);
		return VK_NULL_HANDLE;
#else
		throw NoSuitableMemoryFound("no memory with flags " + std::to_string(uint32_t(flags))
		                            + " could be found and not fallback available");
#endif
	}

	/// @throws vuh::NoSuitableMemoryFound
	static auto findMemory(vuh::Device&, VULKAN_HPP_NAMESPACE::Image&, VULKAN_HPP_NAMESPACE::MemoryPropertyFlags flags
	)-> uint32_t
	{
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(0);
		return VK_NULL_HANDLE;
#else
		throw NoSuitableMemoryFound("no memory with flags " + std::to_string(uint32_t(flags))
		                            + " could be found and not fallback available");
#endif
	}

	/// Create buffer. Normally this should only be called in tests.
	static auto makeBuffer(vuh::Device& device ///< device to create buffer on
	                      , size_t size_bytes  ///< desired size in bytes
	                      , VULKAN_HPP_NAMESPACE::BufferUsageFlags flags ///< additional buffer usage flags
	                      , VULKAN_HPP_NAMESPACE::Result& result
	                      )-> VULKAN_HPP_NAMESPACE::Buffer
	{
		VULKAN_HPP_NAMESPACE::ResultValueType<VULKAN_HPP_NAMESPACE::Buffer>::type buffer = device.createBuffer({ {}, size_bytes, flags});
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		result = buffer.result;
		VULKAN_HPP_ASSERT(VULKAN_HPP_NAMESPACE::Result::eSuccess == result);
		return buffer.value;
#else
		result = VULKAN_HPP_NAMESPACE::Result::eSuccess;
		return buffer;
#endif
	}
	
	/// @throw std::logic_error
	/// Should not normally be called.
	auto memoryProperties(vuh::Device&) const-> VULKAN_HPP_NAMESPACE::MemoryPropertyFlags {
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(0);
		return VULKAN_HPP_NAMESPACE::MemoryPropertyFlags();
#else
		throw std::logic_error("this functions is not supposed to be called");
#endif
	}

	/// @throw std::logic_error
	/// Should not normally be called.
	auto memId() const-> uint32_t {
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		VULKAN_HPP_ASSERT(0);
		return VK_NULL_HANDLE;
#else
		throw std::logic_error("this function is not supposed to be called");
#endif
	}
};

} // namespace arr
} // namespace vuh
