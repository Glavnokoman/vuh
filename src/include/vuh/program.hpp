#pragma once

#include "pipe.h"
#include "device.h"
#include "kernel.hpp"

#include <vulkan/vulkan.hpp>

namespace vuh {
	/// work batch dimensions
	struct Batch {
		uint32_t x;
		uint32_t y = 1;
		uint32_t z = 1;
	};
	
	/// Runnable program. Allows to bind the actual parameters to the interface and execute 
	/// kernel on a Vulkan device.
	template< class Specs
	        , class Params
	        , class Arrays
	        >
	class Program {
	public:
		explicit Program(vuh::Device& device, const Kernel<Specs, Params, Arrays>& kernel) 
		{
			throw "not implemented";
		}

		auto batch(const Batch&)-> Program& {throw "not implemented";}
		auto batch(uint32_t x, uint32_t y = 1, uint32_t z = 1)-> Program& {throw "not implemented";}
		auto bind(const Specs&)-> Program& {throw "not implemented";}
		auto bind(const Params&, Arrays)-> Program& { throw "not implemented";}
		auto bind(const Specs&, const Params&, Arrays)-> Program& {throw "not implemented";}
		auto unbind()-> void {throw "not implemented";}
		auto run() const-> void {throw "not implemented";}
		auto operator()(const Specs&, const Params&, Arrays) const-> void {throw "not implemented";}
	private: // data
		vuh::Pipe _pipe;
		vuh::Device& _device;
		vk::ShaderModule _shader;
		vk::DescriptorSetLayout _dsclayout;
		vk::DescriptorPool _dscpool;
		vk::CommandBuffer _cmdbuffer;
	}; // class Program
} // namespace vuh
