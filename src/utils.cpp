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

        auto genCopyImageToBufferCmd(const vhn::CommandBuffer& transCmdBuf
                , const vhn::Image& im
                , vhn::Buffer& buf
                , const uint32_t imW
                , const uint32_t imH
                , const size_t bufOff
        )-> void
        {
			const int layerCount = 1;
            vhn::Extent3D ext;
            ext.setDepth(1);
            ext.setWidth(imW);
            ext.setHeight(imH);

			vhn::ImageSubresourceLayers imSR;
			imSR.setAspectMask(vhn::ImageAspectFlagBits::eColor);
			imSR.setLayerCount(layerCount);
			imSR.setBaseArrayLayer(0);
			imSR.setMipLevel(0);

            vhn::BufferImageCopy cpyRegion;
            cpyRegion.setBufferOffset(bufOff);
            cpyRegion.setImageExtent(ext);
			cpyRegion.setImageSubresource(imSR);
            transCmdBuf.copyImageToBuffer(im, vhn::ImageLayout::eTransferSrcOptimal, buf, 1, &cpyRegion);
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
            genCopyImageToBufferCmd(transCmdBuf, im, buf, imW, imH, bufOff);
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
            imSR.setMipLevel(0);
			cpyRegion.setImageSubresource(imSR);
			transCmdBuf.copyBufferToImage(buf, im, vhn::ImageLayout::eTransferDstOptimal, regionCount, &cpyRegion);
		}

		auto copyBufferToImage(const vuh::Device& dev
				, const vhn::Buffer& buf
				, vhn::Image& im
				, const vhn::DescriptorType& imDesc
				, const uint32_t imW
				, const uint32_t imH
				, const size_t bufOff
		)-> void
		{
			auto transCmdBuf = const_cast<vuh::Device&>(dev).transferCmdBuffer();
			transCmdBuf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
			genTransImageLayoutCmd(transCmdBuf, im, vhn::ImageLayout::eUndefined, vhn::ImageLayout::eTransferDstOptimal);
			genCopyBufferToImageCmd(transCmdBuf, buf, im, imW, imH, bufOff);
			if (vhn::DescriptorType::eCombinedImageSampler == imDesc) {
				genTransImageLayoutCmd(transCmdBuf, im, vhn::ImageLayout::eTransferDstOptimal,
									   vhn::ImageLayout::eShaderReadOnlyOptimal);
			} else {
				genTransImageLayoutCmd(transCmdBuf, im, vhn::ImageLayout::eTransferDstOptimal,
									   vhn::ImageLayout::eGeneral);
			}
			transCmdBuf.end();
			auto transQueue = const_cast<vuh::Device&>(dev).transferQueue();
			auto submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &transCmdBuf);
			transQueue.submit({submitInfo}, nullptr);
			transQueue.waitIdle();
		}

		//https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkFormat.html
		size_t imageFormatPerPixelBytes(vhn::Format fmt) {
			size_t bytes = 0;
			switch (fmt) {
				case vhn::Format::eR4G4UnormPack8:
					bytes = sizeof(uint8_t);
					break;
				case vhn::Format::eR4G4B4A4UnormPack16:
				case vhn::Format::eB4G4R4A4UnormPack16:
				case vhn::Format::eR5G6B5UnormPack16:
				case vhn::Format::eB5G6R5UnormPack16:
				case vhn::Format::eR5G5B5A1UnormPack16:
				case vhn::Format::eB5G5R5A1UnormPack16:
				case vhn::Format::eA1R5G5B5UnormPack16:
					bytes = sizeof(uint16_t);
					break;
				case vhn::Format::eR8Unorm:
				case vhn::Format::eR8Snorm:
				case vhn::Format::eR8Uscaled:
				case vhn::Format::eR8Sscaled:
				case vhn::Format::eR8Uint:
				case vhn::Format::eR8Sint:
				case vhn::Format::eR8Srgb:
					bytes = sizeof(uint8_t);
					break;
				case vhn::Format::eR8G8Unorm:
				case vhn::Format::eR8G8Snorm:
				case vhn::Format::eR8G8Uscaled:
				case vhn::Format::eR8G8Sscaled:
				case vhn::Format::eR8G8Uint:
				case vhn::Format::eR8G8Sint:
				case vhn::Format::eR8G8Srgb:
					bytes = sizeof(uint16_t);
					break;
				case vhn::Format::eR8G8B8Unorm:
				case vhn::Format::eR8G8B8Snorm:
				case vhn::Format::eR8G8B8Uscaled:
				case vhn::Format::eR8G8B8Sscaled:
				case vhn::Format::eR8G8B8Uint:
				case vhn::Format::eR8G8B8Sint:
				case vhn::Format::eR8G8B8Srgb:
				case vhn::Format::eB8G8R8Unorm:
				case vhn::Format::eB8G8R8Snorm:
				case vhn::Format::eB8G8R8Uscaled:
				case vhn::Format::eB8G8R8Sscaled:
				case vhn::Format::eB8G8R8Uint:
				case vhn::Format::eB8G8R8Sint:
				case vhn::Format::eB8G8R8Srgb:
					bytes = sizeof(uint8_t) * 3;
					break;
				case vhn::Format::eR8G8B8A8Unorm:
				case vhn::Format::eR8G8B8A8Snorm:
				case vhn::Format::eR8G8B8A8Uscaled:
				case vhn::Format::eR8G8B8A8Sscaled:
				case vhn::Format::eR8G8B8A8Uint:
				case vhn::Format::eR8G8B8A8Sint:
				case vhn::Format::eR8G8B8A8Srgb:
				case vhn::Format::eB8G8R8A8Unorm:
				case vhn::Format::eB8G8R8A8Snorm:
				case vhn::Format::eB8G8R8A8Uscaled:
				case vhn::Format::eB8G8R8A8Sscaled:
				case vhn::Format::eB8G8R8A8Uint:
				case vhn::Format::eB8G8R8A8Sint:
				case vhn::Format::eB8G8R8A8Srgb:
				case vhn::Format::eA8B8G8R8UnormPack32:
				case vhn::Format::eA8B8G8R8SnormPack32:
				case vhn::Format::eA8B8G8R8UscaledPack32:
				case vhn::Format::eA8B8G8R8SscaledPack32:
				case vhn::Format::eA8B8G8R8UintPack32:
				case vhn::Format::eA8B8G8R8SintPack32:
				case vhn::Format::eA8B8G8R8SrgbPack32:
				case vhn::Format::eA2R10G10B10UnormPack32:
				case vhn::Format::eA2R10G10B10SnormPack32:
				case vhn::Format::eA2R10G10B10UscaledPack32:
				case vhn::Format::eA2R10G10B10SscaledPack32:
				case vhn::Format::eA2R10G10B10UintPack32:
				case vhn::Format::eA2R10G10B10SintPack32:
				case vhn::Format::eA2B10G10R10UnormPack32:
				case vhn::Format::eA2B10G10R10SnormPack32:
				case vhn::Format::eA2B10G10R10UscaledPack32:
				case vhn::Format::eA2B10G10R10SscaledPack32:
				case vhn::Format::eA2B10G10R10UintPack32:
				case vhn::Format::eA2B10G10R10SintPack32:
					bytes = sizeof(uint32_t);
					break;
				case vhn::Format::eR16Unorm:
				case vhn::Format::eR16Snorm:
				case vhn::Format::eR16Uscaled:
				case vhn::Format::eR16Sscaled:
				case vhn::Format::eR16Uint:
				case vhn::Format::eR16Sint:
				case vhn::Format::eR16Sfloat:
					bytes = sizeof(uint16_t);
					break;
				case vhn::Format::eR16G16Unorm:
				case vhn::Format::eR16G16Snorm:
				case vhn::Format::eR16G16Uscaled:
				case vhn::Format::eR16G16Sscaled:
				case vhn::Format::eR16G16Uint:
				case vhn::Format::eR16G16Sint:
				case vhn::Format::eR16G16Sfloat:
					bytes = sizeof(uint16_t) * 2;
					break;
				case vhn::Format::eR16G16B16Unorm:
				case vhn::Format::eR16G16B16Snorm:
				case vhn::Format::eR16G16B16Uscaled:
				case vhn::Format::eR16G16B16Sscaled:
				case vhn::Format::eR16G16B16Uint:
				case vhn::Format::eR16G16B16Sint:
				case vhn::Format::eR16G16B16Sfloat:
					bytes = sizeof(uint16_t) * 3;
					break;
				case vhn::Format::eR16G16B16A16Unorm:
				case vhn::Format::eR16G16B16A16Snorm:
				case vhn::Format::eR16G16B16A16Uscaled:
				case vhn::Format::eR16G16B16A16Sscaled:
				case vhn::Format::eR16G16B16A16Uint:
				case vhn::Format::eR16G16B16A16Sint:
				case vhn::Format::eR16G16B16A16Sfloat:
					bytes = sizeof(uint16_t) * 4;
					break;
				case vhn::Format::eR32Uint:
				case vhn::Format::eR32Sint:
				case vhn::Format::eR32Sfloat:
					bytes = sizeof(uint32_t);
					break;
				case vhn::Format::eR32G32Uint:
				case vhn::Format::eR32G32Sint:
				case vhn::Format::eR32G32Sfloat:
					bytes = sizeof(uint32_t) * 2;
					break;
				case vhn::Format::eR32G32B32Uint:
				case vhn::Format::eR32G32B32Sint:
				case vhn::Format::eR32G32B32Sfloat:
					bytes = sizeof(uint32_t) * 3;
					break;
				case vhn::Format::eR32G32B32A32Uint:
				case vhn::Format::eR32G32B32A32Sint:
				case vhn::Format::eR32G32B32A32Sfloat:
					bytes = sizeof(uint32_t) * 4;
					break;
				case vhn::Format::eR64Uint:
				case vhn::Format::eR64Sint:
				case vhn::Format::eR64Sfloat:
					bytes = sizeof(uint64_t);
					break;
				case vhn::Format::eR64G64Uint:
				case vhn::Format::eR64G64Sint:
				case vhn::Format::eR64G64Sfloat:
					bytes = sizeof(uint64_t) * 2;
					break;
				case vhn::Format::eR64G64B64Uint:
				case vhn::Format::eR64G64B64Sint:
				case vhn::Format::eR64G64B64Sfloat:
					bytes = sizeof(uint64_t) * 3;
					break;
				case vhn::Format::eR64G64B64A64Uint:
				case vhn::Format::eR64G64B64A64Sint:
				case vhn::Format::eR64G64B64A64Sfloat:
					bytes = sizeof(uint64_t) * 4;
					break;
				case vhn::Format::eB10G11R11UfloatPack32:
				case vhn::Format::eE5B9G9R9UfloatPack32:
					bytes = sizeof(uint32_t);
					break;
				case vhn::Format::eD16Unorm:
					bytes = sizeof(uint16_t);
					break;
				case vhn::Format::eX8D24UnormPack32:
				case vhn::Format::eD32Sfloat:
					bytes = sizeof(uint32_t);
					break;
				case vhn::Format::eS8Uint:
					bytes = sizeof(uint8_t);
					break;
				case vhn::Format::eD16UnormS8Uint:
					bytes = sizeof(uint16_t) + sizeof(uint8_t);
					break;
				case vhn::Format::eD24UnormS8Uint:
					bytes = sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t);
					break;
				case vhn::Format::eD32SfloatS8Uint:
					bytes = sizeof(uint32_t) + sizeof(uint8_t);
					break;
					/*
                     * fix me !!
                     * case vhn::Format::eBc1RgbUnormBlock:
                    case vhn::Format::eBc1RgbSrgbBlock = VK_FORMAT_BC1_RGB_SRGB_BLOCK,
                    case vhn::Format::eBc1RgbaUnormBlock = VK_FORMAT_BC1_RGBA_UNORM_BLOCK,
                    case vhn::Format::eBc1RgbaSrgbBlock = VK_FORMAT_BC1_RGBA_SRGB_BLOCK,
                    case vhn::Format::eBc2UnormBlock = VK_FORMAT_BC2_UNORM_BLOCK,
                    case vhn::Format::eBc2SrgbBlock = VK_FORMAT_BC2_SRGB_BLOCK,
                    case vhn::Format::eBc3UnormBlock = VK_FORMAT_BC3_UNORM_BLOCK,
                    case vhn::Format::eBc3SrgbBlock = VK_FORMAT_BC3_SRGB_BLOCK,
                    case vhn::Format::eBc4UnormBlock = VK_FORMAT_BC4_UNORM_BLOCK,
                    case vhn::Format::eBc4SnormBlock = VK_FORMAT_BC4_SNORM_BLOCK,
                    case vhn::Format::eBc5UnormBlock = VK_FORMAT_BC5_UNORM_BLOCK,
                    case vhn::Format::eBc5SnormBlock = VK_FORMAT_BC5_SNORM_BLOCK,
                    case vhn::Format::eBc6HUfloatBlock = VK_FORMAT_BC6H_UFLOAT_BLOCK,
                    case vhn::Format::eBc6HSfloatBlock = VK_FORMAT_BC6H_SFLOAT_BLOCK,
                    case vhn::Format::eBc7UnormBlock = VK_FORMAT_BC7_UNORM_BLOCK,
                    case vhn::Format::eBc7SrgbBlock = VK_FORMAT_BC7_SRGB_BLOCK,
                    case vhn::Format::eEtc2R8G8B8UnormBlock = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
                    case vhn::Format::eEtc2R8G8B8SrgbBlock = VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
                    case vhn::Format::eEtc2R8G8B8A1UnormBlock = VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
                    case vhn::Format::eEtc2R8G8B8A1SrgbBlock = VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
                    case vhn::Format::eEtc2R8G8B8A8UnormBlock = VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
                    case vhn::Format::eEtc2R8G8B8A8SrgbBlock = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
                    case vhn::Format::eEacR11UnormBlock = VK_FORMAT_EAC_R11_UNORM_BLOCK,
                    case vhn::Format::eEacR11SnormBlock = VK_FORMAT_EAC_R11_SNORM_BLOCK,
                    case vhn::Format::eEacR11G11UnormBlock = VK_FORMAT_EAC_R11G11_UNORM_BLOCK,
                    case vhn::Format::eEacR11G11SnormBlock = VK_FORMAT_EAC_R11G11_SNORM_BLOCK,
                    case vhn::Format::eAstc4x4UnormBlock = VK_FORMAT_ASTC_4x4_UNORM_BLOCK,
                    case vhn::Format::eAstc4x4SrgbBlock = VK_FORMAT_ASTC_4x4_SRGB_BLOCK,
                    case vhn::Format::eAstc5x4UnormBlock = VK_FORMAT_ASTC_5x4_UNORM_BLOCK,
                    case vhn::Format::eAstc5x4SrgbBlock = VK_FORMAT_ASTC_5x4_SRGB_BLOCK,
                    case vhn::Format::eAstc5x5UnormBlock = VK_FORMAT_ASTC_5x5_UNORM_BLOCK,
                    case vhn::Format::eAstc5x5SrgbBlock = VK_FORMAT_ASTC_5x5_SRGB_BLOCK,
                    case vhn::Format::eAstc6x5UnormBlock = VK_FORMAT_ASTC_6x5_UNORM_BLOCK,
                    case vhn::Format::eAstc6x5SrgbBlock = VK_FORMAT_ASTC_6x5_SRGB_BLOCK,
                    case vhn::Format::eAstc6x6UnormBlock = VK_FORMAT_ASTC_6x6_UNORM_BLOCK,
                    case vhn::Format::eAstc6x6SrgbBlock = VK_FORMAT_ASTC_6x6_SRGB_BLOCK,
                    case vhn::Format::eAstc8x5UnormBlock = VK_FORMAT_ASTC_8x5_UNORM_BLOCK,
                    case vhn::Format::eAstc8x5SrgbBlock = VK_FORMAT_ASTC_8x5_SRGB_BLOCK,
                    case vhn::Format::eAstc8x6UnormBlock = VK_FORMAT_ASTC_8x6_UNORM_BLOCK,
                    case vhn::Format::eAstc8x6SrgbBlock = VK_FORMAT_ASTC_8x6_SRGB_BLOCK,
                    case vhn::Format::eAstc8x8UnormBlock = VK_FORMAT_ASTC_8x8_UNORM_BLOCK,
                    case vhn::Format::eAstc8x8SrgbBlock = VK_FORMAT_ASTC_8x8_SRGB_BLOCK,
                    case vhn::Format::eAstc10x5UnormBlock = VK_FORMAT_ASTC_10x5_UNORM_BLOCK,
                    case vhn::Format::eAstc10x5SrgbBlock = VK_FORMAT_ASTC_10x5_SRGB_BLOCK,
                    case vhn::Format::eAstc10x6UnormBlock = VK_FORMAT_ASTC_10x6_UNORM_BLOCK,
                    case vhn::Format::eAstc10x6SrgbBlock = VK_FORMAT_ASTC_10x6_SRGB_BLOCK,
                    case vhn::Format::eAstc10x8UnormBlock = VK_FORMAT_ASTC_10x8_UNORM_BLOCK,
                    case vhn::Format::eAstc10x8SrgbBlock = VK_FORMAT_ASTC_10x8_SRGB_BLOCK,
                    case vhn::Format::eAstc10x10UnormBlock = VK_FORMAT_ASTC_10x10_UNORM_BLOCK,
                    case vhn::Format::eAstc10x10SrgbBlock = VK_FORMAT_ASTC_10x10_SRGB_BLOCK,
                    case vhn::Format::eAstc12x10UnormBlock = VK_FORMAT_ASTC_12x10_UNORM_BLOCK,
                    case vhn::Format::eAstc12x10SrgbBlock = VK_FORMAT_ASTC_12x10_SRGB_BLOCK,
                    case vhn::Format::eAstc12x12UnormBlock = VK_FORMAT_ASTC_12x12_UNORM_BLOCK,
                    case vhn::Format::eAstc12x12SrgbBlock = VK_FORMAT_ASTC_12x12_SRGB_BLOCK
                    case vhn::Format::eG8B8G8R8422Unorm = VK_FORMAT_G8B8G8R8_422_UNORM,
                    case vhn::Format::eG8B8G8R8422UnormKHR = VK_FORMAT_G8B8G8R8_422_UNORM,
                    case vhn::Format::eB8G8R8G8422Unorm = VK_FORMAT_B8G8R8G8_422_UNORM,
                    case vhn::Format::eB8G8R8G8422UnormKHR = VK_FORMAT_B8G8R8G8_422_UNORM,
                    case vhn::Format::eG8B8R83Plane420Unorm = VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
                    case vhn::Format::eG8B8R83Plane420UnormKHR = VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
                    case vhn::Format::eG8B8R82Plane420Unorm = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
                    case vhn::Format::eG8B8R82Plane420UnormKHR = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
                    case vhn::Format::eG8B8R83Plane422Unorm = VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
                    case vhn::Format::eG8B8R83Plane422UnormKHR = VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
                    case vhn::Format::eG8B8R82Plane422Unorm = VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,
                    case vhn::Format::eG8B8R82Plane422UnormKHR = VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,
                    case vhn::Format::eG8B8R83Plane444Unorm = VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
                    case vhn::Format::eG8B8R83Plane444UnormKHR = VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
                    case vhn::Format::eR10X6UnormPack16 = VK_FORMAT_R10X6_UNORM_PACK16,
                    case vhn::Format::eR10X6UnormPack16KHR = VK_FORMAT_R10X6_UNORM_PACK16,
                    case vhn::Format::eR10X6G10X6Unorm2Pack16 = VK_FORMAT_R10X6G10X6_UNORM_2PACK16,
                    case vhn::Format::eR10X6G10X6Unorm2Pack16KHR = VK_FORMAT_R10X6G10X6_UNORM_2PACK16,
                    case vhn::Format::eR10X6G10X6B10X6A10X6Unorm4Pack16 = VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
                    case vhn::Format::eR10X6G10X6B10X6A10X6Unorm4Pack16KHR = VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
                    case vhn::Format::eG10X6B10X6G10X6R10X6422Unorm4Pack16 = VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
                    case vhn::Format::eG10X6B10X6G10X6R10X6422Unorm4Pack16KHR = VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
                    case vhn::Format::eB10X6G10X6R10X6G10X6422Unorm4Pack16 = VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
                    case vhn::Format::eB10X6G10X6R10X6G10X6422Unorm4Pack16KHR = VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
                    case vhn::Format::eG10X6B10X6R10X63Plane420Unorm3Pack16 = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
                    case vhn::Format::eG10X6B10X6R10X63Plane420Unorm3Pack16KHR = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
                    case vhn::Format::eG10X6B10X6R10X62Plane420Unorm3Pack16 = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
                    case vhn::Format::eG10X6B10X6R10X62Plane420Unorm3Pack16KHR = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
                    case vhn::Format::eG10X6B10X6R10X63Plane422Unorm3Pack16 = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
                    case vhn::Format::eG10X6B10X6R10X63Plane422Unorm3Pack16KHR = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
                    case vhn::Format::eG10X6B10X6R10X62Plane422Unorm3Pack16 = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
                    case vhn::Format::eG10X6B10X6R10X62Plane422Unorm3Pack16KHR = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
                    case vhn::Format::eG10X6B10X6R10X63Plane444Unorm3Pack16 = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
                    case vhn::Format::eG10X6B10X6R10X63Plane444Unorm3Pack16KHR = VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
                    case vhn::Format::eR12X4UnormPack16 = VK_FORMAT_R12X4_UNORM_PACK16,
                    case vhn::Format::eR12X4UnormPack16KHR = VK_FORMAT_R12X4_UNORM_PACK16,
                    case vhn::Format::eR12X4G12X4Unorm2Pack16 = VK_FORMAT_R12X4G12X4_UNORM_2PACK16,
                    case vhn::Format::eR12X4G12X4Unorm2Pack16KHR = VK_FORMAT_R12X4G12X4_UNORM_2PACK16,
                    case vhn::Format::eR12X4G12X4B12X4A12X4Unorm4Pack16 = VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
                    case vhn::Format::eR12X4G12X4B12X4A12X4Unorm4Pack16KHR = VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
                    case vhn::Format::eG12X4B12X4G12X4R12X4422Unorm4Pack16 = VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
                    case vhn::Format::eG12X4B12X4G12X4R12X4422Unorm4Pack16KHR = VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
                    case vhn::Format::eB12X4G12X4R12X4G12X4422Unorm4Pack16 = VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
                    case vhn::Format::eB12X4G12X4R12X4G12X4422Unorm4Pack16KHR = VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
                    case vhn::Format::eG12X4B12X4R12X43Plane420Unorm3Pack16 = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
                    case vhn::Format::eG12X4B12X4R12X43Plane420Unorm3Pack16KHR = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
                    case vhn::Format::eG12X4B12X4R12X42Plane420Unorm3Pack16 = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
                    case vhn::Format::eG12X4B12X4R12X42Plane420Unorm3Pack16KHR = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
                    case vhn::Format::eG12X4B12X4R12X43Plane422Unorm3Pack16 = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
                    case vhn::Format::eG12X4B12X4R12X43Plane422Unorm3Pack16KHR = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
                    case vhn::Format::eG12X4B12X4R12X42Plane422Unorm3Pack16 = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
                    case vhn::Format::eG12X4B12X4R12X42Plane422Unorm3Pack16KHR = VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
                    case vhn::Format::eG12X4B12X4R12X43Plane444Unorm3Pack16 = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
                    case vhn::Format::eG12X4B12X4R12X43Plane444Unorm3Pack16KHR = VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
                    case vhn::Format::eG16B16G16R16422Unorm = VK_FORMAT_G16B16G16R16_422_UNORM,
                    case vhn::Format::eG16B16G16R16422UnormKHR = VK_FORMAT_G16B16G16R16_422_UNORM,
                    case vhn::Format::eB16G16R16G16422Unorm = VK_FORMAT_B16G16R16G16_422_UNORM,
                    case vhn::Format::eB16G16R16G16422UnormKHR = VK_FORMAT_B16G16R16G16_422_UNORM,
                    case vhn::Format::eG16B16R163Plane420Unorm = VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
                    case vhn::Format::eG16B16R163Plane420UnormKHR = VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
                    case vhn::Format::eG16B16R162Plane420Unorm = VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,
                    case vhn::Format::eG16B16R162Plane420UnormKHR = VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,
                    case vhn::Format::eG16B16R163Plane422Unorm = VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
                    case vhn::Format::eG16B16R163Plane422UnormKHR = VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
                    case vhn::Format::eG16B16R162Plane422Unorm = VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,
                    case vhn::Format::eG16B16R162Plane422UnormKHR = VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,
                    case vhn::Format::eG16B16R163Plane444Unorm = VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,
                    case vhn::Format::eG16B16R163Plane444UnormKHR = VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,
                    case vhn::Format::ePvrtc12BppUnormBlockIMG = VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG,
                    case vhn::Format::ePvrtc14BppUnormBlockIMG = VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG,
                    case vhn::Format::ePvrtc22BppUnormBlockIMG = VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG,
                    case vhn::Format::ePvrtc24BppUnormBlockIMG = VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG,
                    case vhn::Format::ePvrtc12BppSrgbBlockIMG = VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,
                    case vhn::Format::ePvrtc14BppSrgbBlockIMG = VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,
                    case vhn::Format::ePvrtc22BppSrgbBlockIMG = VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,
                    case vhn::Format::ePvrtc24BppSrgbBlockIMG = VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,*/
			}
			return bytes;
		}
	} // namespace utils
} // namespace vuh
