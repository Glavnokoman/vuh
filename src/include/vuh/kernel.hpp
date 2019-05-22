#pragma once

#include "error.hpp"
#include "traits.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <cassert>
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <tuple>

namespace vuh {

namespace detail {

template<size_t N, std::size_t... I>
auto layout_bindings( const std::array<VkDescriptorType, N>& arr
                    , std::index_sequence<I...>
                    )-> std::array<VkDescriptorSetLayoutBinding, N>
{
	return {VkDescriptorSetLayoutBinding{I, arr[I], 1, VK_SHADER_STAGE_COMPUTE_BIT}...};
}

template<size_t N, std::size_t... I>
auto write_dscsets( VkDescriptorSet dscset
                  , const std::array<VkDescriptorBufferInfo, N>& arr
                  , std::index_sequence<I...>
                  )-> std::array<VkWriteDescriptorSet, N>
{
	return std::array{ VkWriteDescriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr
	                                       , dscset, I, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	                                       , nullptr, &arr[I], nullptr}... };
}

///
template<class Tup, std::size_t... I>
auto specialization_map( const Tup& t
                       , const std::array<std::size_t, sizeof...(I)>& sizes
                       , std::index_sequence<I...>
                       )-> std::array<VkSpecializationMapEntry, sizeof...(I)>
{
	const auto* p = reinterpret_cast<const std::byte*>(&t);
	return std::array{ VkSpecializationMapEntry{I
	                 , uint32_t(reinterpret_cast<const std::byte*>(&std::get<I>(t)) - p)
	                 , sizes[I]}...};
}

} // namespace detail

auto read_spirv(std::string_view path)-> std::vector<std::uint32_t>;

class Device;

/// Helper class to handle the kernel bind parameters.
class BindParameters {
public:
	template<class... Ts>
	explicit BindParameters(VkDevice device, Ts&&... ts){
		// create descriptors set layout
		const auto bindings = detail::layout_bindings(
		                                    std::array{VkDescriptorType{ts.descriptor_class}...}
		                                  , std::index_sequence_for<Ts...>{});
		const auto dsclayout_info = VkDescriptorSetLayoutCreateInfo{
		                            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr
		                            , {} // flags
		                            , uint32_t(bindings.size()), bindings.data() };
		VUH_CHECK(vkCreateDescriptorSetLayout(device, &dsclayout_info, nullptr, &_dsclayout));

		// create descriptors set pool just to hold current descriptors
		const auto sbo_descriptors_size = VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		                                                      , sizeof...(Ts)};
		const auto descriptor_sizes = std::array<VkDescriptorPoolSize, 1>{{sbo_descriptors_size}};
		const auto pool_info = VkDescriptorPoolCreateInfo{
		                       VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr
		                       , {} // flags
		                       , 1
		                       , uint32_t(descriptor_sizes.size()), descriptor_sizes.data() };
		VUH_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &_dscpool));

		// allocate descriptor set from the pool
		const auto dsc_info = VkDescriptorSetAllocateInfo{
		                      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr
		                      , _dscpool
		                      , 1, &_dsclayout };
		VUH_CHECK(vkAllocateDescriptorSets(device, &dsc_info, &_dscset));
	}

	///
	~BindParameters() noexcept {
		vkFreeDescriptorSets(_device, _dscpool, 1, &_dscset);
		vkDestroyDescriptorSetLayout(_device, _dsclayout, nullptr);
		vkDestroyDescriptorPool(_device, _dscpool, nullptr);
	}

	BindParameters(const BindParameters&)=delete;
	auto operator= (const BindParameters&)->BindParameters& = delete;

	/// Actually bind buffer parameters to descriptor set
	template<class... Ts>
	auto bind(traits::DeviceBuffer<Ts>&&... ts)-> void {
		const auto buffer_infos = std::array{
		      VkDescriptorBufferInfo{ts.buffer(), ts.offset_bytes(), ts.size_bytes()}...};
		const auto write_dscsets = detail::write_dscsets(_dscset, buffer_infos
		                                                , std::index_sequence_for<Ts...>{});
		vkUpdateDescriptorSets( _device
		                      , uint32_t(write_dscsets.size()), write_dscsets.data(), 0, nullptr);
	}

	auto descriptors_layout() const-> VkDescriptorSetLayout {return _dsclayout;}
	auto descriptors_set() const-> const VkDescriptorSet& {return _dscset; }
private:
	VkDevice _device;
	VkDescriptorPool _dscpool;         ///< descriptor set pool. Each kernel allocates its own pool to avoid complex dependencies for no benefit.
	VkDescriptorSetLayout _dsclayout;  ///< descriptor set layout. This defines the kernel's array parameters interface.
	VkDescriptorSet _dscset;           ///< descriptors set
}; // class BindParameters

/// Helper class to handle the kernel push parameters.
class PushParameters {
public:
	explicit PushParameters(std::size_t size_bytes);

	auto push(const std::byte* data)-> void;
	auto range() const-> VkPushConstantRange;
	auto values() const-> const std::vector<std::byte>& {return push_params;}
private: // data
	std::vector<std::byte> push_params; ///< bitwise copy of push parameters struct
}; // class PushParameters

/// Helper class to hande kernel specialization parameters.
class SpecParameters {
public:
	template<class... Ts>
	explicit SpecParameters(Ts&&... ts) {
		const auto tmp = std::tuple(ts...);
		const auto* p = reinterpret_cast<const std::byte*>(&tmp);
		data = std::vector<std::byte>(p, p + sizeof(tmp));
		const auto specialization_map = detail::specialization_map(
		                                     tmp
		                                   , std::array<size_t, sizeof...(Ts)>{{sizeof(ts)}...}
		                                   , std::index_sequence_for<Ts...>{});
		info = VkSpecializationInfo{ uint32_t(specialization_map.size()), specialization_map.data()
		                           , data.size(), data.data()};
	}

	auto specialization_info() const-> const VkSpecializationInfo& { return info; }
private: // data
	VkSpecializationInfo  info;
	std::vector<std::byte> data;
}; // class SpecParameters;

///
class Kernel {
public:
	explicit Kernel( Device& device, std::string_view source
	               , std::string entry_point="main", VkShaderModuleCreateFlags flags={});

	~Kernel() noexcept;

	/// Bind buffer (in-out) parameters to the kernel. Can be done multiple times during the
	/// lifetime of the kernel. Causes the command buffer overwrite next time the kernel is queued
	/// for execution. Must correspond exactly to bind interface as defined by the kernel spir-v code.
	template<class... Ts> auto bind(traits::DeviceBuffer<Ts>&&... ts)-> Kernel& {
		_dirty = true;
		if(not _bind){
			_bind = std::make_unique<BindParameters>(_device, std::forward<Ts>(ts)...);
		}
		_bind->bind(std::forward<Ts>(ts)...);
		return *this;
	}

	/// Set push constants of the kernel. Can be done multiple times during the lifetime of the kernel.
	/// Causes the command buffer overwrite next time the kernel is queued for execution.
	/// Must correspond exactly to push constants interface as defined by the kernel spir-v code.
	template<class P> auto push(const P& p)-> Kernel& {
		_dirty = true;
		if(not _push){
			_push = std::make_unique<PushParameters>(sizeof (p));
		}
		_push->push(reinterpret_cast<const std::byte*>(&p));
		return *this;
	}

	/// Set specification constants of the kernel. Can only be done once, before the first kernel run.
	/// Must correspond exactly to specialization constant interface defined in the spir-v code.
	template<class... Ts> auto spec(Ts&&... ts)-> Kernel& {
		assert(not _spec);
		_spec = std::make_unique<SpecParameters>(std::forward<Ts>(ts)...);
		return *this;
	}

	/// set the computation grid dimensions (number of workgroups to run)
	auto grid(std::array<std::uint32_t, 3> dim)-> Kernel& { _grid = dim; return *this; }

	auto command_buffer(VkCommandPool pool)-> VkCommandBuffer;

private: // helpers
	auto init_pipeline()-> void;
	auto record_buffer()-> void;

private: // data
	VkShaderModule _module;
	std::string _entry_point;

	VkCommandBuffer _cmdbuf;           ///< command buffer associated with the kernel
	VkCommandPool   _cmdpool;          ///< non-owning handle to command pool used to allocate the command buffer. Should correspond to the queue family used to run the kernel.
	VkPipelineLayout _pipelayout;      ///< pipeline layout
	VkPipeline _pipeline;              ///< pipeline itself

	vuh::Device& _device;              ///< refer to device to run shader on
	std::unique_ptr<BindParameters> _bind;
	std::unique_ptr<PushParameters> _push;
	std::unique_ptr<SpecParameters> _spec;
	std::array<uint32_t, 3> _grid;     ///< 3D computation grid dimensions (number of workgroups to run)
	bool _dirty;                       ///< flag to indicate the pending writes to command buffer
}; // class Kernel

///
auto run(Kernel& k)-> void;
inline auto run(std::array<uint32_t, 3> grid, Kernel& k)-> void { k.grid(grid); run(k); }
} // namespace vuh
