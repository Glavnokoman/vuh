#include "kernel.hpp"
#include "device.hpp"
#include "error.hpp"

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

/// Creates command buffer based on bind, spec and grid parameters provided before
/// Initializes pipeline if needed.
/// In case the pipeline has been initialized previously the layout is supposed to be compatible
/// with currently pending bind and spec parameters.
void Kernel::make_cmdbuf()
{
	if(not _pipeline){
		init_pipeline();
		VUH_CHECKOUT();
	}
	// Start recording commands into the newly allocated command buffer.
	const auto beginInfo = VkCommandBufferBeginInfo{}; // vk::CommandBufferUsageFlagBits::eOneTimeSubmit
	vkBeginCommandBuffer(_cmdbuf, &beginInfo);

	vkCmdBindPipeline(_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
	vkCmdBindDescriptorSets(_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelayout, 0
	                       , 1, &_bind->descriptors_set()
	                       , 0, nullptr // dynamic offsets
	                       );

	if(const auto& push_constants = _bind->push_constants(); not push_constants.empty()){
		vkCmdPushConstants( _cmdbuf, _pipelayout, VK_SHADER_STAGE_COMPUTE_BIT, 0
		                  , push_constants.size(), push_constants.data());
	}
	vkCmdDispatch(_cmdbuf, _grid[0], _grid[1], _grid[2]);
	vkEndCommandBuffer(_cmdbuf);
}

///
void Kernel::init_pipeline()
{
	const auto ps_ranges = _bind->push_constant_ranges();
	const auto& layout = _bind->descriptors_layout();
	const auto layout_info = VkPipelineLayoutCreateInfo{
	                         VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr
	                         , {} // flags
	                         , 1, &layout
	                         , uint32_t(ps_ranges.size()), ps_ranges.data()};

	VUH_CHECK(vkCreatePipelineLayout(_device, &layout_info, nullptr, &_pipelayout));

	const auto* spec_info = _spec ? &_spec->specialization_info() : nullptr;
	const auto stage_info = VkPipelineShaderStageCreateInfo{
	                        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr
	                        , {} // flags
	                        , VK_SHADER_STAGE_COMPUTE_BIT
	                        , _module
	                        , _entry_point.c_str()
	                        , spec_info };
	const auto pipeline_info = VkComputePipelineCreateInfo{
	                           VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr
	                           , {}
	                           , stage_info
	                           , _pipelayout
	                           , VkPipeline{}, 0 };
	VUH_CHECK(vkCreateComputePipelines( _device, _device.pipeline_cache()
	                                  , 1, &pipeline_info, nullptr, &_pipeline));
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

///
auto run(Kernel& k)-> void
{
	throw "not implemented";
}



} // namespace vuh
