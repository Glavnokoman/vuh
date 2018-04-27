#pragma once

#include "device.h"
#include "string_view.h"

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {
	///
	template<class... Ts> struct typelist{};

	template<class Specs, class Params, class Arrays> class Program;

	/// Brings together shader interfaces declaration and the spirv code.
	/// Needs to be linked to vuh::Device to get the runnable program.
	template< class Specs   ///< tuple of specialization parameters
	        , class Params  ///< shader push parameters interface
	        , class Arrays  ///< arrays_of<> structure of array parameters to shader
	        >
	class Kernel {
	public:
		using specs_t = Specs;
		using array_t = Arrays;
		using params_t = Params;

		explicit Kernel(string_view path, vk::ShaderModuleCreateFlags flags={});
		auto on(vuh::Device& device)-> Program<specs_t, array_t, params_t>;
		auto on(vuh::Device& device, uint32_t queue_id)-> Program<specs_t, array_t, params_t>;
	protected: // data
		std::vector<char> _code;            ///< binary blob with a spirv shader code
		vk::ShaderModuleCreateFlags _flags; ///< shader create flags
	}; // class Kernel
} // namespace vuh
