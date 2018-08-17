#pragma once

#include <vulkan/vulkan.hpp>

#include <type_traits>

namespace vuh {
namespace arr {
/// Contains the traits to hold together buffer and memory flags for 
/// some common buffer usage scenarious. Each trait also defines the fall-back trait, 
/// being void if fall-back is not available.
namespace properties {

	using memflags_t = std::underlying_type_t<vk::MemoryPropertyFlagBits>;
	using bufflags_t = std::underlying_type_t<vk::BufferUsageFlagBits>;

	/// Flags for buffer in host-visible memory.
	/// This is the fallback of most other usage flags, and has no fall-back itself.
	struct Host {
	   using fallback_t = void;
	   static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eHostVisible);
	   static constexpr bufflags_t buffer = {};
	};

	/// Flags for buffer used as a staging buffer to transfer data to GPU.
	struct HostCoherent {
	   using fallback_t = Host;
	   static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eHostVisible )
	                                      | memflags_t(vk::MemoryPropertyFlagBits::eHostCoherent);
	   static constexpr bufflags_t buffer = bufflags_t(vk::BufferUsageFlagBits::eTransferSrc);
	};

	/// Flags for buffer used as a staging buffer to transfer data from GPU.
	struct HostCached {
	   using fallback_t = Host;
	   static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eHostVisible)
	                                      | memflags_t(vk::MemoryPropertyFlagBits::eHostCached );
	   static constexpr bufflags_t buffer = bufflags_t(vk::BufferUsageFlagBits::eTransferDst);
	};

	/// Flags for buffer in both device-local and host-visible memory if such exists.
	/// There is no fall-back for this.
	struct Unified {
	  using fallback_t = void;
	  static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eDeviceLocal)
	                                     | memflags_t(vk::MemoryPropertyFlagBits::eHostVisible);
	  static constexpr bufflags_t buffer = {};
	};

	/// Flags for buffer in device-local memory. Additionally sets transfer bits to enable 
	/// using this buffer to transfer data to/from staging buffers (and eventually host).
	/// The fall-back is Host.
	struct Device {
	   using fallback_t = Host;
	   static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eDeviceLocal);
	   static constexpr bufflags_t buffer = bufflags_t(vk::BufferUsageFlagBits::eTransferSrc)
	                                      | bufflags_t(vk::BufferUsageFlagBits::eTransferDst);
	};

	/// Flags for buffer in device-local memory which is not supposed to take part in data 
	/// exchange with host or other buffers. All you can do with it is to bind it to kernel
	/// as an input or output parameter.
	struct DeviceOnly {
	   using fallback_t = Host;
	   static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eDeviceLocal);
	   static constexpr bufflags_t buffer = {};
	};
} // namespace props
} // namespace arr
} // namespace vuh
