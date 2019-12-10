#include <vuh/utils/utils.h>
#include <vuh/core/vnh.hpp>
#include <vuh/error.h>
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
			throw FileReadFailure(std::string("could not open file ") + fn + " for reading");
#endif
		}
		auto ret = std::vector<char>(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());

		ret.resize(4u*div_up(uint32_t(ret.size()), 4u));
		return ret;
	}

	namespace utils {
		/// Copy data between device buffers using the device transfer command pool and queue.
		/// Source and destination buffers are supposed to be allocated on the same device.
		/// Fully sync, no latency hiding whatsoever.
		auto copyBuf(const vuh::Device& dev ///< device where buffers are allocated
				, vhn::Buffer src    ///< source buffer
				, vhn::Buffer dst    ///< destination buffer
				, size_t szBytes ///< size of memory chunk to copy in bytes
				, size_t srcOff ///< source buffer offset (bytes)
				, size_t dstOff///< destination buffer offset (bytes)
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
            vhn::Extent3D ext;
            ext.setDepth(1);
            ext.setWidth(imW);
            ext.setHeight(imH);

            vhn::BufferImageCopy cpyRegion;
			cpyRegion.setBufferOffset(bufOff);
			cpyRegion.setImageExtent(ext);
			transCmdBuf.copyImageToBuffer(im, vhn::ImageLayout::eTransferSrcOptimal, buf, 1, &cpyRegion);
			transCmdBuf.end();
			auto transQueue = const_cast<vuh::Device&>(dev).transferQueue();
			auto submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &transCmdBuf);
			transQueue.submit({submitInfo}, nullptr);
			transQueue.waitIdle();
		}

		auto genTransImageLayoutCmd(const vhn::CommandBuffer& transCmdBuf
				, const vhn::Image& im
				, const vhn::ImageLayout& lyOld
				, const vhn::ImageLayout& lyNew
		)-> bool
		{
            const int layerCount = 1;

			vhn::ImageMemoryBarrier barrier;
			barrier.setOldLayout(lyOld);
			barrier.setNewLayout(lyNew);
			barrier.setImage(im);
			vhn::ImageSubresourceRange imSR;
			imSR.setAspectMask(vhn::ImageAspectFlagBits::eColor);
			imSR.setBaseMipLevel(0);
			imSR.setLevelCount(1);
			imSR.setBaseArrayLayer(0);
			imSR.setLayerCount(layerCount);
			barrier.setSubresourceRange(imSR);
			barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
			barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

			vhn::PipelineStageFlags srcStageMask;
			vhn::PipelineStageFlags dstStageMask;

			if (vhn::ImageLayout::eUndefined == lyOld && vhn::ImageLayout::eTransferDstOptimal == lyNew) {
				barrier.setSrcAccessMask(vhn::AccessFlags());
				barrier.setDstAccessMask(vhn::AccessFlagBits::eTransferWrite);

				srcStageMask = vhn::PipelineStageFlagBits::eTopOfPipe;
				dstStageMask = vhn::PipelineStageFlagBits::eTransfer;
			} else if (vhn::ImageLayout::eTransferDstOptimal == lyOld && vhn::ImageLayout::eShaderReadOnlyOptimal == lyNew) {
				barrier.setSrcAccessMask(vhn::AccessFlagBits::eTransferWrite);
				barrier.setDstAccessMask(vhn::AccessFlagBits::eTransferRead);

				srcStageMask = vhn::PipelineStageFlagBits::eTransfer;
				dstStageMask = vhn::PipelineStageFlagBits::eTransfer;
			} else {
				return false;
			}

			transCmdBuf.pipelineBarrier(srcStageMask, dstStageMask, vhn::DependencyFlags(), nullptr, nullptr, barrier);
			return true;
		}

		auto genCopyBufferToImageCmd(const vhn::CommandBuffer& transCmdBuf
				, const vhn::Buffer& buf
				, vhn::Image& im
				, const uint32_t imW
				, const uint32_t imH
				, const size_t bufOff
		)-> void
		{
			const int layerCount = 1;
			const int regionCount = 1;
            vhn::Extent3D ext;
            ext.setDepth(1);
            ext.setWidth(imW);
            ext.setHeight(imH);

            vhn::BufferImageCopy cpyRegion;
			cpyRegion.setBufferOffset(bufOff);
			cpyRegion.setImageExtent(ext);
			vhn::ImageSubresourceLayers imSR;
			imSR.setAspectMask(vhn::ImageAspectFlagBits::eColor);
			imSR.setLayerCount(layerCount);
            imSR.setBaseArrayLayer(0);
			cpyRegion.setImageSubresource(imSR);
			transCmdBuf.copyBufferToImage(buf, im, vhn::ImageLayout::eTransferDstOptimal, regionCount, &cpyRegion);
		}

		auto copyBufferToImage(const vuh::Device& dev
				, const vhn::Buffer& buf
				, vhn::Image& im
				, const uint32_t imW
				, const uint32_t imH
				, const size_t bufOff
		)-> void
		{
			auto transCmdBuf = const_cast<vuh::Device&>(dev).transferCmdBuffer();
			transCmdBuf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
			genTransImageLayoutCmd(transCmdBuf, im, vhn::ImageLayout::eUndefined, vhn::ImageLayout::eTransferDstOptimal);
			genCopyBufferToImageCmd(transCmdBuf, buf, im, imW, imH, bufOff);
			genTransImageLayoutCmd(transCmdBuf, im, vhn::ImageLayout::eTransferDstOptimal, vhn::ImageLayout::eShaderReadOnlyOptimal);
			transCmdBuf.end();
			auto transQueue = const_cast<vuh::Device&>(dev).transferQueue();
			auto submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &transCmdBuf);
			transQueue.submit({submitInfo}, nullptr);
			transQueue.waitIdle();
		}
	} // namespace utils
} // namespace vuh
