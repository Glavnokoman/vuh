#pragma once

#include "error.hpp"
#include "traits.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <vector>
#include <string>
#include <string_view>

namespace vuh {

auto read_spirv(std::string_view path)-> std::vector<std::uint32_t>;

class Device;

///
class BindParameters {
public:
	template<class... Ts>
	explicit BindParameters(VkDevice device, Ts&&... ts){
		// create descriptors set layout
		using indicies = std::index_sequence_for<Ts...>;
		const auto bindings = std::array{
		                      VkDescriptorSetLayoutBinding{ indicies{}, Ts::descriptor_class
		                                                  , 1, VK_SHADER_STAGE_COMPUTE_BIT}...};
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

		bind(ts...);
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
	auto bind(VkDevice device, traits::DeviceBuffer<Ts>&&... ts)-> void {
		const auto ids = std::index_sequence_for<Ts...>{};
		const auto write_dscsets = std::array{{_dscset, ids, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		      , nullptr, VkDescriptorBufferInfo{ts.buffer(), ts.offset_bytes(), ts.size_bytes()}}...};
		vkUpdateDescriptorSets(device, 1, &write_dscsets, 0, nullptr);
	}

	auto descriptors_layout() const-> VkDescriptorSetLayout {return _dsclayout;}
	auto descriptors_set() const-> const VkDescriptorSet& {return _dscset; }
private:
	VkDevice _device;
	VkDescriptorPool _dscpool;         ///< descriptor set pool. Each kernel allocates its own pool to avoid complex dependencies for no benefit.
	VkDescriptorSetLayout _dsclayout;  ///< descriptor set layout. This defines the kernel's array parameters interface.
	VkDescriptorSet _dscset;           ///< descriptors set
}; // class BindParameters

///
class PushParameters {
public:
	PushParameters(const std::byte* data, const std::size_t size_bytes);

	auto push(const std::byte* data, const std::size_t size_bytes)-> void;
	auto range() const-> VkPushConstantRange;
	auto values() const-> const std::vector<std::byte>& {return push_params;}
private: // data
	std::vector<std::byte> push_params; ///< bitwise copy of push parameters struct
}; // class PushParameters

///
class SpecParameters {
public:
	template<class... Ts>
	SpecParameters() { throw "not implemented"; }

	auto specialization_info() const-> const VkSpecializationInfo&;
private: // data
}; // class SpecParameters;

///
class Kernel {
public:
	explicit Kernel( Device& device, std::string_view source
	               , std::string entry_point="main", VkShaderModuleCreateFlags flags={});

	template<class... Ts> auto bind(traits::DeviceBuffer<Ts>&&...)-> Kernel& {
		throw "not implemented";
	}
	template<class P> auto push(const P& p)-> Kernel& {throw "not implemented";}
	template<class... Ts> auto spec(Ts&&...)-> Kernel& {throw "not implemented";}
	auto grid(std::array<std::uint32_t, 3> dim)-> Kernel& {throw "not implemented";}

	auto make_cmdbuf()-> void;

private: // helpers
	auto init_pipeline()-> void;

private: // data
	VkShaderModule _module;
	std::string _entry_point;

	VkCommandBuffer _cmdbuf;           ///< command buffer associated with the kernel
	VkPipelineLayout _pipelayout;      ///< pipeline layout
	mutable VkPipeline _pipeline;      ///< pipeline itself

	vuh::Device& _device;              ///< refer to device to run shader on
	std::unique_ptr<BindParameters> _bind;
	std::unique_ptr<PushParameters> _push;
	std::unique_ptr<SpecParameters> _spec;
	std::array<uint32_t, 3> _grid;     ///< 3D computation grid dimensions (number of workgroups to run)
}; // class Kernel

///
inline auto run(Kernel& k)-> void;
inline auto run(std::array<uint32_t, 3> grid, Kernel& k)-> void { k.grid(grid); run(k); }
} // namespace vuh
