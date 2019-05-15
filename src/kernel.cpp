#include "kernel.hpp"
#include "device.hpp"

#include <algorithm>
#include <fstream>

namespace vuh {

///
Kernel::Kernel( Device& device
              , std::string_view source
              , std::string entry_point
              , VkShaderModuleCreateFlags flags)
   : _entry_point(std::move(entry_point))
   , _device{device}
{
	const auto code = read_spirv(source);
	const auto create_info = VkShaderModuleCreateInfo {
	                         VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr
	                         , flags
	                         , code.size()*sizeof(std::uint32_t)
	                         , code.data() };
	vkCreateShaderModule(device, &create_info, nullptr, &_module);
}

///
auto read_spirv(std::string_view path)-> std::vector<std::uint32_t> {
	auto fin = std::ifstream(path.data(), std::ios::binary | std::ios::ate);
	if(!fin.is_open()){
		throw std::runtime_error(std::string("failed opening file ") + path.data() + " for reading");
	}
	const auto stream_size = unsigned(fin.tellg());
	fin.seekg(0);

	auto ret = std::vector<std::uint32_t>((stream_size + 3)/4, 0);
	std::copy( std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>()
	         , reinterpret_cast<char*>(ret.data()));
	return ret;
}


} // namespace vuh
