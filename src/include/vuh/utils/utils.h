#pragma once

#include <cstdint>
#include <vector>
#include <vuh/device.h>

namespace vuh {
	/// Typelist. That is all about it.
	template<class... Ts> struct typelist{};

	/// @return nearest integer bigger or equal to exact division value
	inline auto div_up(uint32_t x, uint32_t y){ return (x + y - 1u)/y; }

	/// Read binary shader file into array of uint32_t. little endian assumed.
	/// Padded by 0s to a boundary of 4.
	auto read_spirv(const char* fn)-> std::vector<char>;

	namespace utils {
		/// Copy data between device buffers using the device transfer command pool and queue.
		/// Source and destination buffers are supposed to be allocated on the same device.
		/// Fully sync, no latency hiding whatsoever.
		auto copyBuf(const vuh::Device& dev ///< device where buffers are allocated
				, vhn::Buffer src    ///< source buffer
				, vhn::Buffer dst    ///< destination buffer
				, size_t szBytes ///< size of memory chunk to copy in bytes
				, size_t srcOff = 0 ///< source buffer offset (bytes)
				, size_t dstOff = 0///< destination buffer offset (bytes)
		)-> void;

		auto copyImageToBuffer(const vuh::Device& dev
				, const vhn::Image& im
				, vhn::Buffer& buf
				, const uint32_t imW
				, const uint32_t imH
				, const size_t bufOff = 0
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
				, const size_t bufOff = 0
		)-> void;

		auto copyBufferToImage(const vuh::Device& dev
				, const vhn::Buffer& buf
				, vhn::Image& im
				, const vhn::DescriptorType& imDesc
                , const uint32_t imW
				, const uint32_t imH
				, const size_t bufOff = 0
		)-> void;

		// image format bytes per Pixel
		size_t imageFormatPerPixelBytes(vhn::Format fmt);
	} // namespace utils
} // namespace vuh
