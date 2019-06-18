#pragma once

#include <vulkan/vulkan_core.h>

namespace vuh::error {
/// vuh specific error codes
enum VuhError: int32_t {
	  NoComputeCapableQueueFamilySpecified = VK_RESULT_END_RANGE + 1
	, NoTransferCapableQueueFamilySpecified
	, NoSuitableMemoryFound
};

auto text(VkResult)-> const char*;
auto text(VuhError)-> const char*;


} // namespace vuh::error
