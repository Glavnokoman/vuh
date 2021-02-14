#include "kernel.hpp"
#include "device.hpp"
#include "error.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>

namespace vuh {

///
Kernel::Kernel( Device& device
              , std::string_view source
              , std::string entry_point
              , VkShaderModuleCreateFlags flags)
   : _entry_point(std::move(entry_point))
   , _cmdbuf{nullptr}
   , _cmdpool{nullptr}
   , _pipeline{nullptr}
   , _device{device}
{
	const auto code = read_spirv(source);
	const auto create_info = VkShaderModuleCreateInfo {
	                         VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr
	                         , flags
	                         , code.size()*sizeof(decltype(code)::value_type) // byte size
	                         , code.data() };
	VUH_CHECK(vkCreateShaderModule(device, &create_info, nullptr, &_module));
}

///
Kernel::Kernel( Device& device
              , size_t byte_size
              , const uint32_t source[]
              , std::string entry_point
              , VkShaderModuleCreateFlags flags)
   : _entry_point(std::move(entry_point))
   , _cmdbuf{nullptr}
   , _cmdpool{nullptr}
   , _pipeline{nullptr}
   , _device{device}
{
	const auto create_info = VkShaderModuleCreateInfo {
	                         VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr
	                         , flags
	                         , byte_size
	                         , source };
	VUH_CHECK(vkCreateShaderModule(device, &create_info, nullptr, &_module));
}

///
Kernel::~Kernel() noexcept
{
	vkDestroyPipeline(_device, _pipeline, nullptr);
	vkDestroyPipelineLayout(_device, _pipelayout, nullptr);
	if(_cmdpool){ vkFreeCommandBuffers(_device, _cmdpool, 1, &_cmdbuf); }
	vkDestroyShaderModule(_device, _module, nullptr);
}

///
Kernel::Kernel(Kernel&& other) noexcept
    : _module{other._module}
    , _entry_point{std::move(other._entry_point)}
    , _cmdbuf{other._cmdbuf}
    , _cmdpool{other._cmdpool}
    , _pipelayout{other._pipelayout}
    , _pipeline{other._pipeline}
    , _device{other._device}
    , _bind{std::move(other._bind)}
    , _push{std::move(other._push)}
    , _spec{std::move(other._spec)}
    , _grid{other._grid}
    , _dirty{other._dirty}
{
    other._cmdbuf = nullptr;
    other._pipelayout = nullptr;
    other._pipeline = nullptr;
    other._dirty = false;
}

///
auto Kernel::operator=(Kernel&& other) noexcept-> Kernel& {
    assert(_device == other._device);
    std::swap(_module, other._module);
    std::swap(_entry_point, other._entry_point);
    std::swap(_cmdbuf, other._cmdbuf);
    std::swap(_cmdpool, other._cmdpool);
    std::swap(_pipelayout, other._pipelayout);
    std::swap(_pipeline, other._pipeline);
    std::swap(_bind, other._bind);
    std::swap(_push, other._push);
    std::swap(_spec, other._spec);
    std::swap(_grid, other._grid);
    std::swap(_dirty, other._dirty);
    return *this;
}

/// Creates command buffer based on bind, spec and grid parameters provided before
/// Initializes pipeline if needed.
/// In case the pipeline has been initialized previously the layout is supposed to be compatible
/// with currently pending bind and spec parameters.
auto Kernel::command_buffer(VkCommandPool pool)-> const VkCommandBuffer& {
	if(_dirty && _cmdbuf){
		VUH_CHECK(vkResetCommandBuffer(_cmdbuf, {})); // do not release associated resources, they will be reused
		record_buffer();
	}
	if(not _pipeline){
		assert(_cmdbuf == nullptr && _cmdpool == nullptr);
		init_pipeline();
		VUH_CHECKOUT();
		_cmdpool = pool;
		const auto buffer_info = VkCommandBufferAllocateInfo{
		                         VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr
		                         , _cmdpool
		                         , VK_COMMAND_BUFFER_LEVEL_PRIMARY
		                         , 1  // command buffer count
		};
		VUH_CHECK(vkAllocateCommandBuffers(_device, &buffer_info, &_cmdbuf));
		record_buffer();
	}

	return _cmdbuf;
}

///
auto Kernel::init_pipeline()-> void {
	assert(_bind);
	const auto ps_ranges = _push ? std::vector{{_push->range()}} : std::vector<VkPushConstantRange>{};
	const auto& layout = _bind->descriptors_layout();
	const auto layout_info = VkPipelineLayoutCreateInfo{
	                         VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, nullptr
	                         , {} // flags
	                         , 1, &layout
	                         , uint32_t(ps_ranges.size()), ps_ranges.data()};

	VUH_CHECK(vkCreatePipelineLayout(_device, &layout_info, nullptr, &_pipelayout));

	const auto spec_info = _spec ? _spec->specialization_info()
	                             : VkSpecializationInfo{0, nullptr, 0, nullptr};
	const auto stage_info = VkPipelineShaderStageCreateInfo{
	                        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr
	                        , {} // flags
	                        , VK_SHADER_STAGE_COMPUTE_BIT
	                        , _module
	                        , _entry_point.c_str()
	                        , &spec_info };
	const auto pipeline_info = VkComputePipelineCreateInfo{
	                           VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, nullptr
	                           , {}
	                           , stage_info
	                           , _pipelayout
	                           , VkPipeline{}, 0 };
	VUH_CHECK(vkCreateComputePipelines( _device, _device.pipeline_cache()
	                                    , 1, &pipeline_info, nullptr, &_pipeline));
}

/// record command into command buffer. Resets _dirty flag to false.
/// @pre buffer is in initial state
/// @post buffer is in executable state
auto Kernel::record_buffer()-> void {
	_dirty = false;
	const auto begin_info = VkCommandBufferBeginInfo{
	                  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr
	                  , {} // usage flags
	                  , nullptr};
	vkBeginCommandBuffer(_cmdbuf, &begin_info);

	vkCmdBindPipeline(_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline);
	vkCmdBindDescriptorSets(_cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE, _pipelayout, 0
	                       , 1, &_bind->descriptors_set()
	                       , 0, nullptr // dynamic offsets
	                       );
	if(_push){
		const auto& push_constants = _push->values();
		assert(not push_constants.empty());
		vkCmdPushConstants( _cmdbuf, _pipelayout, VK_SHADER_STAGE_COMPUTE_BIT, 0
		                  , push_constants.size(), push_constants.data());
	}
	vkCmdDispatch(_cmdbuf, _grid[0], _grid[1], _grid[2]);
	vkEndCommandBuffer(_cmdbuf);
}

/// doc me
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
PushParameters::PushParameters(std::size_t size_bytes)
   : push_params(size_bytes)
{}

///
void PushParameters::push(const std::byte* data)
{
	std::copy_n(data, push_params.size(), push_params.data());
}

///
VkPushConstantRange PushParameters::range() const
{
	return VkPushConstantRange{VK_SHADER_STAGE_COMPUTE_BIT, 0, uint32_t(push_params.size())};
}

} // namespace vuh
