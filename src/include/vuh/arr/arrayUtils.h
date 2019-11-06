#pragma once

#include "vuh/core/core.hpp"
#include "vuh/device.h"

namespace vuh{
namespace arr {
	auto copyBuf(const vuh::Device& dev
	             , vhn::Buffer src
	             , vhn::Buffer dst
	             , size_t size_bytes
	             , size_t src_offset=0
	             , size_t dst_offset=0
	             )-> void;

	auto copyBufferToImage(const vuh::Device& dev
			, const vhn::Buffer& src
			, vhn::Image& dst
			, const uint32_t imageWidth
			, const uint32_t imageHeight
			, const size_t bufferOffset=0
	)-> void;

	auto copyImageToBuffer(const vuh::Device& dev
			, const vhn::Image& src
			, vhn::Buffer& dst
			, const uint32_t imageWidth
			, const uint32_t imageHeight
			, const size_t bufferOffset=0
	)-> void;
} // namespace arr
} // namespace vuh
