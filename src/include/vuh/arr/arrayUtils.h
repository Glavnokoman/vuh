#pragma once

#include "vuh/core/core.hpp"
#include "vuh/device.h"

namespace vuh{
namespace arr {
	auto copyBuf(vuh::Device& device
	             , vhn::Buffer src
	             , vhn::Buffer dst
	             , size_t size_bytes
	             , size_t src_offset=0
	             , size_t dst_offset=0
	             )-> void;

	auto copyBufferToImage(vuh::Device& device
			, vhn::Buffer src
			, vhn::Image dst
			, uint32_t imageWidth
			, uint32_t imageHeight
			, size_t bufferOffset=0
	)-> void;

	auto copyImageToBuffer(vuh::Device& device
			, vhn::Image src
			, vhn::Buffer dst
			, uint32_t imageWidth
			, uint32_t imageHeight
			, size_t bufferOffset=0
	)-> void;
} // namespace arr
} // namespace vuh
