#pragma once

#include "pipe.h"
#include "device.h"
#include "array.hpp"

#include <vulkan/vulkan.hpp>

namespace {

} // namespace

namespace vuh {
	///
	template<class... Ts> struct typelist{};

	/// work batch dimensions
	struct Batch {
		uint32_t x;
		uint32_t y = 1;
		uint32_t z = 1;
	};
	
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
//			auto config = std::vector<vk::DescriptorType>(sizeof...(Ts)
//			                                              , vk::DescriptorType::eStorageBuffer); // for now only storage buffers supported
			_shader = device.createShaderModule(code, flags);
			_dscpool = device.allocDescriptorPool(
			                                {{vk::DescriptorType::eStorageBuffer, sizeof...(Ts)}}, 1);
			auto bind_layout = std::array<vk::DescriptorSetLayoutBinding, sizeof...(Ts)>{
				              {0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}};
			_dsclayout = device.makeDescriptorsLayout();
			throw "not implemented";
		}

		auto batch(const Batch&)-> Program& {throw "not implemented";}
		auto batch(uint32_t x, uint32_t y = 1, uint32_t z = 1)-> Program& {throw "not implemented";}
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
		vuh::Pipe _pipe;
		vuh::Device& _device;
//		vk::CommandBuffer _cmdbuffer;
	}; // class Program
} // namespace vuh
