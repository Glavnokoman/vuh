#pragma once

#include "traits.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <vector>
#include <string>
#include <string_view>

namespace vuh {

auto read_spirv(std::string_view path)-> std::vector<std::uint32_t>;

class Device;

///
class Kernel {
public:
	explicit Kernel( Device& device, std::string_view source
	               , std::string entry_point="main", VkShaderModuleCreateFlags flags={});

	template<class... Ts> auto spec(Ts&&...)-> Kernel& {throw "not implemented";}
	template<class P, class... Ts> auto bind(const P& p, traits::DeviceBuffer<Ts>&...)-> Kernel& {throw "not implemented";}
//	template<class P, class T1, class T2> auto bind(const P& p, traits::DeviceBuffer<T1>& b1
//	                                      , traits::DeviceBuffer<T2>& b2)-> Kernel& {throw "not implemented";}

	auto grid(const std::array<std::uint32_t, 3>& dim)-> Kernel& {throw "not implemented";}
private: // data
	VkShaderModule _module;
	std::string _entry_point;

	VkCommandBuffer _commands;         ///< command buffer associated with the kernel
	VkDescriptorPool _dscpool;         ///< descriptor set pool. Each kernel allocates its own pool to avoid complex dependencies for no benefit.
	VkDescriptorSetLayout _dsclayout;  ///< descriptor set layout. This defines the kernel's array parameters interface.
	VkDescriptorSet _dscset;           ///< descriptors set
	VkPipelineLayout _pipelayout;      ///< pipeline layout
	mutable VkPipeline _pipeline;      ///< pipeline itself

	vuh::Device& _device;                ///< refer to device to run shader on
	std::array<uint32_t, 3> _batch={0, 0, 0}; ///< 3D evaluation grid dimensions (number of workgroups to run)
}; // class Kernel

///
inline auto run(Kernel& k)-> void {throw "not implemented";}
inline auto run(std::array<uint32_t, 3> grid, Kernel& k)-> void {throw "not implemented";}
} // namespace vuh
