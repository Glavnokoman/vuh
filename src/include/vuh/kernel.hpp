#pragma once

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
	template<class T>
	explicit BindParameters(){throw "not implemented";}

	auto push_constant_ranges() const-> std::vector<VkPushConstantRange>;
	auto push_constants() const-> const std::vector<std::byte>;
	auto descriptors_layout() const-> VkDescriptorSetLayout {return _dsclayout;}
	auto descriptors_set() const-> const VkDescriptorSet& {return _dscset; }
private:
	VkDescriptorPool _dscpool;         ///< descriptor set pool. Each kernel allocates its own pool to avoid complex dependencies for no benefit.
	VkDescriptorSetLayout _dsclayout;  ///< descriptor set layout. This defines the kernel's array parameters interface.
	VkDescriptorSet _dscset;           ///< descriptors set

	std::vector<std::byte> push_params; ///< bit-copy of push params
}; // class BindParameters

///
class SpecParameters {
public:
	auto specialization_info() const-> const VkSpecializationInfo&;

}; // class SpecParameters;

///
class Kernel {
public:
	explicit Kernel( Device& device, std::string_view source
	               , std::string entry_point="main", VkShaderModuleCreateFlags flags={});

	template<class... Ts> auto spec(Ts&&...)-> Kernel& {throw "not implemented";}
	template<class P, class... Ts> auto bind( const traits::NotDeviceBuffer<P>& p
	                                        , traits::DeviceBuffer<Ts>&&...)-> Kernel& {throw "not implemented";}
	template<class... Ts> auto bind(traits::DeviceBuffer<Ts>&&...)-> Kernel& {throw "not implemented";}
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
	std::array<uint32_t, 3> _grid={0, 0, 0}; ///< 3D computation grid dimensions (number of workgroups to run)
	std::unique_ptr<BindParameters> _bind;
	std::unique_ptr<SpecParameters> _spec;
}; // class Kernel

///
inline auto run(Kernel& k)-> void;
inline auto run(std::array<uint32_t, 3> grid, Kernel& k)-> void { k.grid(grid); run(k); }
} // namespace vuh
