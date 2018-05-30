#pragma once

#include "vuh/device.h"

#include <vulkan/vulkan.hpp>

namespace vuh{
namespace arr {
	auto copyBuf(vuh::Device& device
	             , vk::Buffer src, vk::Buffer dst
	             , uint32_t size_bytes
	             )-> void;
} // namespace arr
} // namespace vuh
