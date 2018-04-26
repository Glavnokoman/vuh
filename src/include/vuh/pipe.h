#pragma once

#include <vulkan/vulkan.hpp>

namespace vuh {
	/// doc me
	struct Pipe {
		explicit Pipe(vk::Device device);
	// data
		vk::PipelineCache cache;
		vk::PipelineLayout layout;
		vk::Pipeline pipeline;
	}; // class Pipe
} // namespace vuh
