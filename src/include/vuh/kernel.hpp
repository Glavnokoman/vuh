#pragma once

#include "device.h"
#include "program.hpp"

#include <vulkan/vulkan.hpp>

#include <vector>
#include <iostream>
#include <fstream>

namespace {
	auto div_up(uint32_t x, uint32_t y){ return (x + y - 1u)/y; }

	/// Read binary shader file into array of uint32_t. little endian assumed.
	/// Padded by 0s to a boundary of 4.
	auto read_spirv(const char* filename)-> std::vector<char> {
		auto fin = std::ifstream(filename, std::ios::binary);
		if(!fin.is_open()){
			throw std::runtime_error(std::string("could not open file ") + filename);
		}
		auto ret = std::vector<char>(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());

		ret.resize(4u*div_up(uint32_t(ret.size()), 4u));
		return ret;
	}

} // namespace

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
		explicit Kernel(const char* path, vk::ShaderModuleCreateFlags flags={})
		   : _code(read_spirv(path))
		   , _flags(flags)
		{}
		
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
