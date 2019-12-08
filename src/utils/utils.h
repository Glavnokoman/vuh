#pragma once

#include <cstdint>
#include <vector>
#include <vuh/core/vnh.hpp>
#include <vuh/device.h>

namespace vuh {
	/// Typelist. That is all about it.
	template<class... Ts> struct typelist{};

	/// @return nearest integer bigger or equal to exact division value
	inline auto div_up(uint32_t x, uint32_t y){ return (x + y - 1u)/y; }

	auto read_spirv(const char* filename)-> std::vector<char>;

	namespace utils {
		auto copyBuf(const vuh::Device& dev
				, vhn::Buffer src
				, vhn::Buffer dst
				, size_t szBytes
				, size_t srcOff=0
				, size_t dstOff=0
		)-> void;

		auto genTransImageLayoutCmd(const vhn::CommandBuffer& transCmdBuf
				, const vhn::Image& im
				, const vhn::ImageLayout& lyOld
				, const vhn::ImageLayout& lyNew
		)-> bool;

		auto genCopyBufferToImageCmd(const vhn::CommandBuffer& transCmdBuf
				, const vhn::Buffer& buf
				, vhn::Image& im
				, const uint32_t imW
				, const uint32_t imH
				, const size_t bufOff=0
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
