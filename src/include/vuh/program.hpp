#pragma once

#include "kernel.hpp"
#include "pipe.h"
#include "device.h"

#include <vulkan/vulkan.hpp>

namespace vuh {
	///
	template<class Specs
	         , class Params
	         , class Arrays
	         >
	class Program {
	public:
		explicit Program(vuh::Device& device, const Kernel<Specs, Params, Arrays>& kernel);

		auto with(const Specs&) const-> const Program&;
		auto operator[](const Specs&) const-> const Program&;
		auto bind(const Params&, Arrays) const-> const Program&;
		auto unbind() const-> void;
		auto run() const-> void;
		auto operator()(const Params&, Arrays) const-> void;
	private: // data
		vuh::Pipe _pipe;
		vk::ShaderModule _shader;
		vk::DescriptorSetLayout _dsclayout;
		vk::DescriptorPool _dscpool;
		vk::CommandBuffer _cmdbuffer;
		vuh::Device& _device;
	}; // class Program
} // namespace vuh
