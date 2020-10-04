#include <vuh/utils.h>
#include <vuh/error.h>
#include <vuh/arr/arrayUtils.h>

#include <fstream>
#include <iterator>

namespace vuh {
	
	/// Read binary shader file into array of uint32_t. little endian assumed.
	auto read_spirv(const char* filename)-> std::vector<uint32_t> {
		auto fin = std::ifstream(filename, std::ios::binary | std::ios::ate);
		if(!fin.is_open()){
			throw std::runtime_error(std::string("failed opening file ") + filename + " for reading");
		}
		const auto stream_size = unsigned(fin.tellg());
		fin.seekg(0);

		auto ret = std::vector<std::uint32_t>((stream_size + 3)/4, 0);
		std::copy( std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>()
	         	  , reinterpret_cast<char*>(ret.data()));
		return ret;
	}

namespace arr {
	/// Copy data between device buffers using the device transfer command pool and queue.
	/// Source and destination buffers are supposed to be allocated on the same device.
	/// Fully sync, no latency hiding whatsoever.
	auto copyBuf(vuh::Device& device ///< device where buffers are allocated
	             , vk::Buffer src    ///< source buffer
	             , vk::Buffer dst    ///< destination buffer
	             , size_t size_bytes ///< size of memory chunk to copy in bytes
	             , size_t src_offset ///< source buffer offset (bytes)
	             , size_t dst_offset ///< destination buffer offset (bytes)
	             )-> void
	{
		auto cmd_buf = device.transferCmdBuffer();
		cmd_buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		auto region = vk::BufferCopy(src_offset, dst_offset, size_bytes);
		cmd_buf.copyBuffer(src, dst, region);
		cmd_buf.end();
		auto queue = device.transferQueue();
		auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buf);
		queue.submit({submit_info}, nullptr);
		queue.waitIdle();
	}
} // namespace arr
} // namespace vuh
