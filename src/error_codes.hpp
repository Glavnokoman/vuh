#pragma once

#include <vulkan/vulkan_core.h>

namespace vuh::error {
/// vuh specific error codes
enum VuhError: int32_t {
	  NoComputeCapableQueueFamilySpecified = VK_RESULT_END_RANGE + 1
	, NoTransferCapableQueueFamilySpecified
};

/// messages for vulkan and vuh error codes
inline auto text(VkResult)-> const char* {
	return "TBD";
}

/// messages for vuh specific errors
inline auto text(VuhError)-> const char* {
	return "TBD";
}


} // namespace vuh::error
