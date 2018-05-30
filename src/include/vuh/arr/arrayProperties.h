#pragma once

#include <vulkan/vulkan.hpp>

#include <type_traits>

namespace vuh {
namespace arr {
namespace properties {

	using memflags_t = std::underlying_type_t<vk::MemoryPropertyFlagBits>;
	using bufflags_t = std::underlying_type_t<vk::BufferUsageFlagBits>;

	///
	struct Host {
	   using fallback_t = void;
	   static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eHostVisible);
	   static constexpr bufflags_t buffer = {};
	};

	///
	struct HostStage {
	   using fallback_t = Host;
	   static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eHostVisible )
	                                      | memflags_t(vk::MemoryPropertyFlagBits::eHostCoherent);
	   static constexpr bufflags_t buffer = bufflags_t(vk::BufferUsageFlagBits::eTransferSrc);
	};

	///
	struct HostCached {
	   using fallback_t = Host;
	   static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eHostVisible)
	                                      | memflags_t(vk::MemoryPropertyFlagBits::eHostCached );
	   static constexpr bufflags_t buffer = bufflags_t(vk::BufferUsageFlagBits::eTransferDst);
	};

	///
	struct Unified {
	  using fallback_t = void;
	  static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eDeviceLocal)
	                                     | memflags_t(vk::MemoryPropertyFlagBits::eHostVisible);
	  static constexpr bufflags_t buffer = {};
	};

	///
	struct Device {
	   using fallback_t = Host;
	   static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eDeviceLocal);
	   static constexpr bufflags_t buffer = bufflags_t(vk::BufferUsageFlagBits::eTransferSrc)
	                                      | bufflags_t(vk::BufferUsageFlagBits::eTransferDst);
	};

	///
	struct DeviceOnly {
	   using fallback_t = Host;
	   static constexpr memflags_t memory = memflags_t(vk::MemoryPropertyFlagBits::eDeviceLocal);
	   static constexpr bufflags_t buffer = {};
	};
} // namespace props
} // namespace arr
} // namespace vuh
