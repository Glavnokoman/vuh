#pragma once

#include "device.h"
#include "string_view.h"
#include "program.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {
	template<class Specs, class Params, class A> class Program;

	/// Brings together shader interfaces declaration and the spirv code.
	/// Needs to be linked to vuh::Device to get the runnable program.
	/// Serves as a proxy object for typelist to pack of its types conversion.
	template< class Specs   ///< tuple of specialization parameters
	        , class Params  ///< shader push parameters interface
	        , class Arrays  ///< typelist of value types of array parameters
           >
	class Kernel {
	public:
		explicit Kernel(string_view path, vk::ShaderModuleCreateFlags flags={}) 
		{
			throw "not implemented";
		}
		
		///
		auto on(vuh::Device& device)-> Program<Specs, Params, Arrays> {
			throw "not implemented";
		}
		
		///
		auto on(vuh::Device& device, uint32_t queue_id)-> Program<Specs, Params, Arrays> {
			throw "not implemented";
		}
	protected: // data
		std::vector<char> _code;            ///< binary blob with a spirv shader code
		vk::ShaderModuleCreateFlags _flags; ///< shader create flags
	}; // class Kernel
} // namespace vuh
