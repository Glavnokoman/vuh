#pragma once

#include "pipe.h"
#include "device.h"
#include "array.hpp"

#include <vulkan/vulkan.hpp>

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
	
	/// specialization to unpack 
	template< class Specs  ///< tuple of specialization parameters
	        , class Params ///< shader push parameters structure
	        , template<class...> class Arrays ///< typelist of value types of array parameters
	        , class... Ts  ///< pack of value types of array parameters
	        >
	class Program<Specs, Params, Arrays<Ts...>> {
	public:
		explicit Program(vuh::Device& device, std::vector<char> code, vk::ShaderModuleCreateFlags flags) 
		{
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
		vuh::Pipe _pipe;
		vuh::Device& _device;
		vk::ShaderModule _shader;
		vk::DescriptorSetLayout _dsclayout;
		vk::DescriptorPool _dscpool;
		vk::CommandBuffer _cmdbuffer;
	}; // class Program
} // namespace vuh
