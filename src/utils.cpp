#include <vuh/utils.h>
#include <vuh/error.h>
#include <vuh/arr/arrayUtils.h>

#include <fstream>

namespace vuh {
	/// Read binary shader file into array of uint32_t. little endian assumed.
	/// Padded by 0s to a boundary of 4.
	auto read_spirv(const char* filename)-> std::vector<char> {
		auto fin = std::ifstream(filename, std::ios::binary);
		if(!fin.is_open()){
			throw FileReadFailure(std::string("could not open file ") + filename + " for reading");
		}
		auto ret = std::vector<char>(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());

		ret.resize(4u*div_up(uint32_t(ret.size()), 4u));
		return ret;
	}

namespace arr {
	/// Copy device buffers using the transient command pool.
	/// Fully sync, no latency hiding whatsoever.
	auto copyBuf(vuh::Device& device
	             , vk::Buffer src, vk::Buffer dst
	             , uint32_t size  ///< size of memory chunk to copy in bytes
	             )-> void
	{
		auto cmd_buf = device.transferCmdBuffer();
		cmd_buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		auto region = vk::BufferCopy(0, 0, size);
		cmd_buf.copyBuffer(src, dst, 1, &region);
		cmd_buf.end();
		auto queue = device.transferQueue();
		auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buf);
		queue.submit({submit_info}, nullptr);
		queue.waitIdle();
	}
} // namespace arr
} // namespace vuh
