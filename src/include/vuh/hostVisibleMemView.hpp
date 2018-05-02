#pragma once

#include "device.h"

#include <vulkan/vulkan.hpp>

namespace vuh {
	// Helper class to access the host-visible device memory from the host.
	template<class T>
	class HostVisibleMemView {
	public:
		using pointer_type = T*;

		/// Constructor
		HostVisibleMemView( vuh::Device& device
		                    , vk::DeviceMemory memory
		                    , uint32_t n_elements ///< number of elements
		                    )
			: _begin(static_cast<pointer_type>(device.mapMemory(memory, 0, n_elements*sizeof(T))))
			, _end(_begin + n_elements)
		{}

		auto begin()-> pointer_type { return _begin; }
		auto end()-> pointer_type { return _end; }

	private: // data
		const pointer_type _begin;  ///< points to the first element
		const pointer_type _end;    ///< points to the element one past the end of the mapped memory chunk
	}; // class HostVisibleMemView
} // namespace vuh
