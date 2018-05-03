#pragma once

#include "device.h"
#include "array.hpp"

#include <vulkan/vulkan.hpp>

namespace {

	namespace detail {
		template<class T> struct DictTypeToDsc;
	
		template<> struct DictTypeToDsc<float>{ 
			static constexpr vk::DescriptorType value = vk::DescriptorType::eStorageBuffer;
		};
		
		template<> struct DictTypeToDsc<uint32_t>{ 
			static constexpr vk::DescriptorType value = vk::DescriptorType::eStorageBuffer; 
		};
		
		/// Not currently used, just a wild idea on how to support uniforms (and dynamic uniforms),
		/// needs check if that works. If not - descriptor types have to be passed as a parameter 
		/// to kernel constructor.
		template<> struct DictTypeToDsc<const float>{ 
			static constexpr vk::DescriptorType value = vk::DescriptorType::eUniformBuffer;
		};
	} // namespace trais

	/// doc me
	template<template<class...> class L, class... Ts>
	auto typesToDscTypes() {
		return std::array<vk::DescriptorType, sizeof...(Ts)>{detail::DictTypeToDsc<Ts>::value...};
	}
	
	/// doc me
	template<size_t N>
	auto dscTypesToLayout(const std::array<vk::DescriptorType, N>& dsc_types, uint32_t layout_id) {
		auto r = std::array<vk::DescriptorSetLayoutBinding, N>{};
		for(size_t i = 0; i < N; ++i){ // this can be done compile-time, but hardly worth it.
			r[i] = {i, dsc_types[i], layout_id, vk::ShaderStageFlagBits::eCompute};
		}
		return r;
	}
} // namespace

namespace vuh {
	///
	template<class... Ts> struct typelist{};

	/// Runnable program. Allows to bind the actual parameters to the interface and execute 
	/// kernel on a Vulkan device.
	template<class S, class P, class A> class Program;
	
	/// specialization to unpack array types parameters
	template< class Specs  ///< tuple of specialization parameters
	        , class Params ///< shader push parameters structure
	        , template<class...> class Arrays ///< typelist of value types of array parameters
	        , class... Ts  ///< pack of value types of array parameters
	        >
	class Program<Specs, Params, Arrays<Ts...>> {
	public:
		explicit Program(vuh::Device& device, const std::vector<char>& code
		                 , vk::ShaderModuleCreateFlags flags
		                 )
		   : _device(device)
		{
			auto dscTypes = typesToDscTypes<Arrays<Ts...>>();
			_shader = device.createShaderModule(code, flags);
			_dscpool = device.allocDescriptorPool(dscTypes, 1);
			_dsclayout = device.makeDescriptorsLayout(dscTypesToLayout(dscTypes));
			_pipecache = device.createPipeCache();
			auto push_constant_range = vk::PushConstantRange(vk::ShaderStageFlagBits::eCompute
			                                                 , 0, sizeof(Params));
			_pipelayout = device.createPipelineLayout({_dsclayout}, {push_constant_range});
			// the other structures specified at binding time
		}

		auto batch(uint32_t x, uint32_t y = 1, uint32_t z = 1)-> Program& {
			throw "not implemented";
		}
		auto bind(const Specs&)-> Program& {throw "not implemented";}
		
		auto bind(const Params&, vuh::Array<Ts>&... ars)-> Program& { throw "not implemented";}
		auto bind(const Specs&, const Params&, vuh::Array<Ts>&... ars)-> Program& {throw "not implemented";}
		auto unbind()-> void {throw "not implemented";}
		auto run() const-> void {throw "not implemented";}
		auto operator()(const Specs&, const Params&, vuh::Array<Ts>&... ars) const-> void {throw "not implemented";}
	private: // data
		vk::ShaderModule _shader;
		vk::DescriptorPool _dscpool;
		vk::DescriptorSetLayout _dsclayout;
		vk::PipelineCache _pipecache;
		vk::PipelineLayout _pipelayout;
		mutable vk::Pipeline _pipeline;
		mutable vk::CommandBuffer _cmdbuffer;
		std::array<uint32_t, 3> _batch={0, 0, 0};
		vuh::Device& _device;
	}; // class Program
} // namespace vuh
