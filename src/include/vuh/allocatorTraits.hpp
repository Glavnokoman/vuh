#pragma once

#include <vulkan/vulkan.h>
//#include <vulkan/vulkan.hpp>

//#include <type_traits>

/// Contains the traits to hold together buffer and memory flags for
/// some common buffer usage scenarious.
/// Each trait also defines the fall-back allocation trait
/// being void if the fall-back is not available.
namespace vuh::allocator::traits {
	using memflags_t = VkFlags; //std::underlying_type_t<VkMemoryPropertyFlagBits>;
	using bufflags_t = VkFlags;

	/// Flags for buffer in host-visible memory.
	/// This is the fallback of most other usage flags, and has no fall-back itself.
	struct Host {
	   using fallback_t = void;
	   static constexpr auto memory = memflags_t(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	   static constexpr auto buffer = bufflags_t{};
	};

	/// Flags for buffer in a host-accessible memory with coherent access. 
	/// Best used as a staging buffer to transfer data to GPU.
	struct HostCoherent {
	   using fallback_t = Host;
	   static constexpr auto memory = memflags_t( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	                                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	   static constexpr auto buffer = bufflags_t(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	};

	/// Flags for buffer in a host-accessible memory with cached access. 
	/// Best used as a staging buffer to transfer data from GPU.
	struct HostCached {
	   using fallback_t = Host;
	   static constexpr auto memory = memflags_t( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
	                                            | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	   static constexpr auto buffer = bufflags_t(VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	};

	/// Flags for buffer in both device-local and host-visible memory if such exists.
	/// There is no fall-back for this.
	struct Unified {
	  using fallback_t = void;
	  static constexpr auto memory = memflags_t( VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	                                           | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	  static constexpr auto buffer = bufflags_t{};
	};

	/// Flags for buffer in device-local memory. Additionally sets transfer bits to enable 
	/// using this buffer to transfer data to/from staging buffers (and eventually host).
	/// The fall-back is Host.
	struct Device {
	   using fallback_t = Host;
	   static constexpr auto memory = memflags_t(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	   static constexpr auto buffer = bufflags_t( VK_BUFFER_USAGE_TRANSFER_SRC_BIT
	                                            | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	};

	/// Flags for buffer in device-local memory which is not supposed to take part in data 
	/// exchange with host or other buffers. All you can do with it is to bind it to kernel
	/// as an input or output parameter.
	struct DeviceOnly {
	   using fallback_t = Host;
	   static constexpr auto memory = memflags_t(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	   static constexpr auto buffer = bufflags_t{};
	};
} // namespace vuh::allocator::traits
