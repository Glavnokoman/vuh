#pragma once

#include "vuh/device.h"

#include <vulkan/vulkan.hpp>

namespace vuh{
namespace arr {
	auto copyBuf(vuh::Device& device
	             , VULKAN_HPP_NAMESPACE::Buffer src
	             , VULKAN_HPP_NAMESPACE::Buffer dst
	             , size_t size_bytes
	             , size_t src_offset=0
	             , size_t dst_offset=0
	             )-> void;
} // namespace arr
} // namespace vuh
