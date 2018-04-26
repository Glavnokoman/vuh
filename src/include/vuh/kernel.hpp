#pragma once

#include "string_view.h"

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {
	///
	template<class... Ts> struct arrays_of{};

	/// Brings together shader interfaces declaration and the spirv code.
	/// Needs to be linked to vuh::Device to get the runnable program.
	template<class specs_t    ///< tuple of specialization parameters
	         , class arrays_t ///< arrays_of<> structure of array parameters to shader
	         , class params_t ///< shader push parameters interface
	         >
	class Kernel {
	public:
		using specs_t = specs_t;
		using array_t = arrays_t;
		using params_t = params_t;

		explicit Kernel(string_view path, vk::ShaderModuleCreateFlags flags={});

	protected: // data
		std::vector<char> _code;            ///< binary blob with a spirv shader code
		vk::ShaderModuleCreateFlags _flags; ///< shader create flags
	}; // class Kernel
} // namespace vuh
