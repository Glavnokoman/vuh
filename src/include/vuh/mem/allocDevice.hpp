#pragma once

#include "vuh/core/core.hpp"
#include <vuh/device.h>
#include <vuh/error.h>
#include <vuh/instance.h>
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
			static auto makeBuffer(const vuh::Device& dev   ///< device to create buffer on
								  , size_t size_bytes    ///< desired size in bytes
								  , vhn::BufferUsageFlags flags ///< additional (to the ones defined in Props) buffer usage flags
								  , vhn::Result& res
								  )-> vhn::Buffer {
				const auto flags_combined = flags | vhn::BufferUsageFlags(Props::buffer);

				auto buffer = dev.createBuffer({ {}, size_bytes, flags_combined});
		#ifdef VULKAN_HPP_NO_EXCEPTIONS
				res = buffer.result;
				VULKAN_HPP_ASSERT(vhn::Result::eSuccess == res);
				return buffer.value;
		#else
				res = vhn::Result::eSuccess;
				return buffer;
		#endif
			}

			/// Create image on a device.
			static auto makeImage(const vuh::Device& dev   ///< device to create buffer on
					, vhn::ImageType imageType
					, size_t width    ///< desired width
					, size_t height    ///< desired height
					, vhn::Format fmt
					, vhn::ImageUsageFlags flags ///< additional (to the ones defined in Props) image usage flags
					, vhn::Result& res
			)-> vhn::Image {
				const auto flags_combined = flags | vhn::ImageUsageFlags(Props::image);

				vhn::Extent3D extent;
				extent.setWidth(width);
				extent.setHeight(height);
				extent.setDepth(1);

				vhn::ImageCreateInfo imageCreateInfo;
				imageCreateInfo.setImageType(imageType);
				imageCreateInfo.setFormat(fmt);
				imageCreateInfo.setExtent(extent);
				imageCreateInfo.setUsage(flags_combined);
				imageCreateInfo.setTiling(vhn::ImageTiling::eOptimal);

				auto image = dev.createImage(imageCreateInfo);
		#ifdef VULKAN_HPP_NO_EXCEPTIONS
				res = image.result;
				VULKAN_HPP_ASSERT(vhn::Result::eSuccess == res);
				return image.value;
		#else
				res = vhn::Result::eSuccess;
				return image;
		#endif
			}

			/// Create image on a device.
			static auto makeImageView(const vuh::Device& dev   ///< device to create buffer on
					, vhn::Image image
					, vhn::ImageViewType viewType
					, vhn::Format fmt
					, vhn::Result& res
			)-> vhn::ImageView {
				vhn::ImageViewCreateInfo imageViewCreateInfo;
				imageViewCreateInfo.setImage(image);
				imageViewCreateInfo.setViewType(viewType);
				imageViewCreateInfo.setFormat(fmt);

				auto imageView = dev.createImageView(imageViewCreateInfo);
#ifdef VULKAN_HPP_NO_EXCEPTIONS
				res = imageView.result;
				VULKAN_HPP_ASSERT(vhn::Result::eSuccess == res);
				return imageView.value;
#else
				res = vhn::Result::eSuccess;
				return imageView;
#endif
			}

			/// Allocate memory for the image.
			auto allocMemory(const vuh::Device& dev  ///< device to allocate memory
					, vhn::Image image  ///< buffer to allocate memory for
					, vhn::MemoryPropertyFlags flags_mem ///< additional (to the ones defined in Props) memory property flags
					, vhn::Result& res
			)-> vhn::DeviceMemory {
				auto mem_id = findMemory(dev, image, flags_mem);
				auto mem = vhn::DeviceMemory{};
		#ifdef VULKAN_HPP_NO_EXCEPTIONS
				auto alloc_mem = dev.allocateMemory({dev.getImageMemoryRequirements(image).size, mem_id});
				VULKAN_HPP_ASSERT(vhn::Result::eSuccess == alloc_mem.result);
                res = alloc_mem.result;
				if (vhn::Result::eSuccess == alloc_mem.result) {
					mem = alloc_mem.value;
				} else {
					dev.instance().report("AllocDevice failed to allocate memory, using fallback", vhn::to_string(alloc_mem.result).c_str()
							, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
		#else
				try{
					mem = dev.allocateMemory({dev.getImageMemoryRequirements(image).size, mem_id});
					res = vhn::Result::eSuccess;
				} catch (vhn::Error& e){
					dev.instance().report("AllocDevice failed to allocate memory, using fallback", e.what()
											 , VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
		#endif
					auto allocFallback = AllocFallback{};
					mem = allocFallback.allocMemory(dev, image, flags_mem, res);
				}

				return mem;
			}

			/// Allocate memory for the buffer.
			auto allocMemory(const vuh::Device& dev  ///< device to allocate memory
							 , vhn::Buffer buffer  ///< buffer to allocate memory for
							 , vhn::MemoryPropertyFlags flags_mem ///< additional (to the ones defined in Props) memory property flags
							 , vhn::Result& res
							 )-> vhn::DeviceMemory {
				auto mem_id = findMemory(dev, buffer, flags_mem);
				auto mem = vhn::DeviceMemory{};
		#ifdef VULKAN_HPP_NO_EXCEPTIONS
				auto alloc_mem = dev.allocateMemory({dev.getBufferMemoryRequirements(buffer).size, mem_id});
				VULKAN_HPP_ASSERT(vhn::Result::eSuccess == alloc_mem.result);
				if (vhn::Result::eSuccess == alloc_mem.result) {
					mem = alloc_mem.value;
				} else {
					dev.instance().report("AllocDevice failed to allocate memory, using fallback", vhn::to_string(alloc_mem.result).c_str()
							, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
		#else
				try{
					mem = dev.allocateMemory({dev.getBufferMemoryRequirements(buffer).size, mem_id});
				} catch (vhn::Error& e){
					dev.instance().report("AllocDevice failed to allocate memory, using fallback", e.what()
											 , VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
		#endif
					auto allocFallback = AllocFallback{};
					mem = allocFallback.allocMemory(dev, buffer, flags_mem, res);
				}

				return mem;
			}

			/// @return memory property flags of the memory on which actual allocation took place.
			auto memoryProperties(const vuh::Device& dev, const vhn::Buffer& buffer, const vhn::MemoryPropertyFlags flags_mem) const-> vhn::MemoryPropertyFlags {
                uint32_t mem_id = findMemory(dev, buffer, flags_mem);
                return dev.memoryProperties(mem_id);
			}

            auto memoryProperties(const vuh::Device& dev, const vhn::Image& image, const vhn::MemoryPropertyFlags flags_mem) const-> vhn::MemoryPropertyFlags {
                uint32_t mem_id = findMemory(dev, image, flags_mem);
                return dev.memoryProperties(mem_id);
            }

			/// @return id of the first memory matched requirements of the given buffer and Props
			/// If requirements are not matched memory properties defined in Props are relaxed to
			/// those of the fallback.
			/// This may cause for example allocation in host-visible memory when device-local
			/// was originally requested but not available on a given device.
			/// This would only be reported through reporter associated with Instance, and no error
			/// raised.
			static auto findMemory(const vuh::Device& dev ///< device on which to search for suitable memory
								   , const vhn::Buffer& buffer       ///< buffer to find suitable memory for
								   , const vhn::MemoryPropertyFlags flags_mem={} ///< additional memory flags
								   )-> uint32_t {
				auto mem_id = dev.selectMemory(buffer
										   , vhn::MemoryPropertyFlags(vhn::MemoryPropertyFlags(Props::memory)
											 | flags_mem));
				if(mem_id != uint32_t(-1)){
					return mem_id;
				}
				dev.instance().report("AllocDevice could not find desired memory type, using fallback", " "
										 , VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
				return AllocFallback::findMemory(dev, buffer, flags_mem);
			}

			/// @return id of the first memory match requirements of the given image and Props
			/// If requirements are not matched memory properties defined in Props are relaxed to
			/// those of the fallback.
			/// This may cause for example allocation in host-visible memory when device-local
			/// was originally requested but not available on a given device.
			/// This would only be reported through reporter associated with Instance, and no error
			/// raised.
			static auto findMemory(const vuh::Device& dev ///< device on which to search for suitable memory
					, const vhn::Image& image       ///< image to find suitable memory for
					, const vhn::MemoryPropertyFlags flags_mem={} ///< additional memory flags
			)-> uint32_t {
				auto mem_id = dev.selectMemory(image
						, vhn::MemoryPropertyFlags(vhn::MemoryPropertyFlags(Props::memory)
																	| flags_mem));
				if(mem_id != uint32_t(-1)){
					return mem_id;
				}
				dev.instance().report("AllocDevice could not find desired memory type, using fallback", " "
						, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT);
				return AllocFallback::findMemory(dev, image, flags_mem);
			}
		}; // class AllocDevice

		/// Specialize allocator for void properties type.
		/// Calls to this class methods basically means all other failed and nothing can be done.
		/// This means most its methods throw exceptions.
		template<>
		class AllocDevice<void>{
		public:
			using properties_t = void;

			/// @throws vbk::OutOfDeviceMemoryError
			auto allocMemory(const vuh::Device&, const vhn::Buffer&, const vhn::MemoryPropertyFlags, vhn::Result& res)-> vhn::DeviceMemory {
		    #ifdef VULKAN_HPP_NO_EXCEPTIONS
				VULKAN_HPP_ASSERT(0);
                res = vhn::Result::eErrorOutOfDeviceMemory;
				return vhn::DeviceMemory();
		    #else
				throw vhn::OutOfDeviceMemoryError("failed to allocate device memory"
												 " and no fallback available");
		    #endif
			}

			/// @throws vbk::OutOfDeviceMemoryError
			auto allocMemory(const vuh::Device&, const vhn::Image&, const vhn::MemoryPropertyFlags, vhn::Result& res)-> vhn::DeviceMemory {
		    #ifdef VULKAN_HPP_NO_EXCEPTIONS
				VULKAN_HPP_ASSERT(0);
                res = vhn::Result::eErrorOutOfDeviceMemory;
				return vhn::DeviceMemory();
		    #else
				throw vhn::OutOfDeviceMemoryError("failed to allocate device memory"
												 " and no fallback available");
		    #endif
			}

			/// @throws vuh::NoSuitableMemoryFound
			static auto findMemory(const vuh::Device&, const vhn::Buffer&, const vhn::MemoryPropertyFlags flags
								   )-> uint32_t {
		    #ifdef VULKAN_HPP_NO_EXCEPTIONS
				VULKAN_HPP_ASSERT(0);
				return VK_NULL_HANDLE;
		    #else
				throw NoSuitableMemoryFound("no memory with flags " + std::to_string(uint32_t(flags))
											+ " could be found and not fallback available");
		    #endif
			}

			/// @throws vuh::NoSuitableMemoryFound
			static auto findMemory(const vuh::Device&, const vhn::Image&, const vhn::MemoryPropertyFlags flags
			)-> uint32_t {
		    #ifdef VULKAN_HPP_NO_EXCEPTIONS
                VULKAN_HPP_ASSERT(0);
                return VK_NULL_HANDLE;
		    #else
                throw NoSuitableMemoryFound("no memory with flags " + std::to_string(uint32_t(flags))
                                            + " could be found and not fallback available");
		    #endif
			}

            /// Create buffer. Normally this should only be called in tests.
            static auto makeBuffer(const vuh::Device& dev ///< device to create buffer on
                                  , const size_t size_bytes  ///< desired size in bytes
                                  , const vhn::BufferUsageFlags flags ///< additional buffer usage flags
                                  , vhn::Result& res
                                  )-> vhn::Buffer {
                vhn::ResultValueType<vhn::Buffer>::type buffer = dev.createBuffer({ {}, size_bytes, flags});
		    #ifdef VULKAN_HPP_NO_EXCEPTIONS
                res = buffer.result;
                VULKAN_HPP_ASSERT(vhn::Result::eSuccess == res);
                return buffer.value;
		    #else
                res = vhn::Result::eSuccess;
                return buffer;
		    #endif
            }

            /// @throw std::logic_error
            /// Should not normally be called.
            auto memoryProperties(const vuh::Device&, const vhn::Buffer&, const vhn::MemoryPropertyFlags) const-> vhn::MemoryPropertyFlags {
		    #ifdef VULKAN_HPP_NO_EXCEPTIONS
                VULKAN_HPP_ASSERT(0);
                return vhn::MemoryPropertyFlags();
		    #else
                throw std::logic_error("this functions is not supposed to be called");
		    #endif
            }

        };
	} // namespace arr
} // namespace vuh
