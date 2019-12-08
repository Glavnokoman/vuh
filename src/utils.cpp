#include <vuh/utils.h>
#include <vuh/error.h>
#include <vuh/arr/arrayUtils.h>

#include <fstream>

namespace vuh {
	/// Read binary shader file into array of uint32_t. little endian assumed.
	/// Padded by 0s to a boundary of 4.
	auto read_spirv(const char* fn)-> std::vector<char> {
		auto fin = std::ifstream(fn, std::ios::binary);
		if(!fin.is_open()){
#ifdef VULKAN_HPP_NO_EXCEPTIONS
			VULKAN_HPP_ASSERT(0);
			return std::vector<char>();
#else
			throw FileReadFailure(std::string("could not open file ") + filename + " for reading");
#endif
		}
		auto ret = std::vector<char>(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());

		ret.resize(4u*div_up(uint32_t(ret.size()), 4u));
		return ret;
	}

namespace arr {
	/// Copy data between device buffers using the device transfer command pool and queue.
	/// Source and destination buffers are supposed to be allocated on the same device.
	/// Fully sync, no latency hiding whatsoever.
	auto copyBuf(const vuh::Device& dev ///< device where buffers are allocated
	             , vhn::Buffer src    ///< source buffer
	             , vhn::Buffer dst    ///< destination buffer
	             , size_t szBytes ///< size of memory chunk to copy in bytes
	             , size_t srcOff ///< source buffer offset (bytes)
	             , size_t dstOff ///< destination buffer offset (bytes)
	             )-> void
	{
		auto transCmdBuf = const_cast<vuh::Device&>(dev).transferCmdBuffer();
		transCmdBuf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		auto region = vk::BufferCopy(srcOff, dstOff, szBytes);
		transCmdBuf.copyBuffer(src, dst, 1, &region);
		transCmdBuf.end();
		auto transQueue = const_cast<vuh::Device&>(dev).transferQueue();
		auto submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &transCmdBuf);
		transQueue.submit({submitInfo}, nullptr);
		transQueue.waitIdle();
	}

	auto copyImageToBuffer(const vuh::Device& dev
			, const vhn::Image& im
			, vhn::Buffer& buf
			, const uint32_t imW
			, const uint32_t imH
			, const size_t bufOff
	)-> void
	{
		auto transCmdBuf = const_cast<vuh::Device&>(dev).transferCmdBuffer();
		transCmdBuf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		vhn::BufferImageCopy cpyRegion;
		cpyRegion.setBufferOffset(bufOff);
		cpyRegion.setImageExtent({imW, imH, 1});
		transCmdBuf.copyImageToBuffer(im, vhn::ImageLayout::eTransferSrcOptimal, buf, 1, &cpyRegion);
		transCmdBuf.end();
		auto transQueue = const_cast<vuh::Device&>(dev).transferQueue();
		auto submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &transCmdBuf);
		transQueue.submit({submitInfo}, nullptr);
		transQueue.waitIdle();
	}

	auto copyBufferToImage(const vuh::Device& dev
			, const vhn::Buffer& buf
			, vhn::Image& im
			, const uint32_t imW
			, const uint32_t imH
			, const size_t bufOff
	)-> void
	{
		const int layerCount = 1;
		const int regionCount = 1;
		auto transCmdBuf = const_cast<vuh::Device&>(dev).transferCmdBuffer();
		transCmdBuf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		vhn::BufferImageCopy cpyRegion;
		cpyRegion.setBufferOffset(bufOff);
		cpyRegion.setImageExtent({imW, imH, 1});
		vhn::ImageSubresourceLayers imSR;
		imSR.setAspectMask(vhn::ImageAspectFlagBits::eColor);
		imSR.setLayerCount(layerCount);
		cpyRegion.setImageSubresource(imSR);
		transCmdBuf.copyBufferToImage(buf, im, vhn::ImageLayout::eTransferDstOptimal, regionCount, &cpyRegion);
		transCmdBuf.end();
		auto transQueue = const_cast<vuh::Device&>(dev).transferQueue();
		auto submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &transCmdBuf);
		transQueue.submit({submitInfo}, nullptr);
		transQueue.waitIdle();
	}
} // namespace arr
} // namespace vuh
