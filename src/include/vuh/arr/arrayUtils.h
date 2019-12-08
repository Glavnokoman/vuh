#pragma once

#include "vuh/core/core.hpp"
#include "vuh/device.h"

namespace vuh{
namespace arr {
	auto copyBuf(const vuh::Device& dev
	             , vhn::Buffer src
	             , vhn::Buffer dst
	             , size_t szBytes
	             , size_t srcOff=0
	             , size_t dstOff=0
	             )-> void;

	auto copyBufferToImage(const vuh::Device& dev
			, const vhn::Buffer& buf
			, vhn::Image& im
			, const uint32_t imW
			, const uint32_t imH
			, const size_t bufOff=0
	)-> void;

	auto copyImageToBuffer(const vuh::Device& dev
			, const vhn::Image& im
			, vhn::Buffer& buf
			, const uint32_t imW
			, const uint32_t imH
			, const size_t bufOff=0
	)-> void;
} // namespace arr
} // namespace vuh
