#pragma once

#include <vulkan/vulkan_core.h>

namespace vuh::error {
/// messages for vulkan and vuh error codes
inline auto text(VkResult)-> const char* {
	return "TBD";
}

/// vuh specific error codes
enum VuhError: int32_t {
	  NoComputeCapableQueueFamilySpecified = VK_RESULT_END_RANGE + 1
	, NoTransferCapableQueueFamilySpecified
};

} // namespace vuh::error
