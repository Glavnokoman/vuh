#include "error_codes.hpp"

namespace vuh::error {

/// messages for vulkan and error codes
auto text(VkResult err)-> const char* {
	switch(err){
		case VK_NOT_READY:                      return "vk_not_ready";
		case VK_TIMEOUT:                        return "vk_timeout";
		case VK_EVENT_SET:                      return "vk_event_set";
		case VK_EVENT_RESET:                    return "vk_event_reset";
		case VK_INCOMPLETE:                     return "vk_incomplete";
		case VK_ERROR_OUT_OF_HOST_MEMORY:       return "vk_error_out_of_host_memory";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:     return "vk_error_out_of_device_memory";
		case VK_ERROR_INITIALIZATION_FAILED:    return "vk_error_initialization_failed";
		case VK_ERROR_DEVICE_LOST:              return "vk_error_device_lost";
		case VK_ERROR_MEMORY_MAP_FAILED:        return "vk_error_memory_map_failed";
		case VK_ERROR_LAYER_NOT_PRESENT:        return "vk_error_layer_not_present";
		case VK_ERROR_EXTENSION_NOT_PRESENT:    return "vk_error_extension_not_present";
		case VK_ERROR_FEATURE_NOT_PRESENT:      return "vk_error_feature_not_present";
		case VK_ERROR_INCOMPATIBLE_DRIVER:      return "vk_error_incompatible_driver";
		case VK_ERROR_TOO_MANY_OBJECTS:         return "vk_error_too_many_objects";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:     return "vk_error_format_not_supported";
		case VK_ERROR_FRAGMENTED_POOL:          return "vk_error_fragmented_pool";
		case VK_ERROR_OUT_OF_POOL_MEMORY:       return "vk_error_out_of_pool_memory";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:  return "vk_error_invalid_external_handle";
		case VK_ERROR_SURFACE_LOST_KHR:         return "vk_error_surface_lost_khr";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "vk_error_native_window_in_use_khr";
		case VK_SUBOPTIMAL_KHR:                 return "vk_suboptimal_khr";
		case VK_ERROR_OUT_OF_DATE_KHR:          return "vk_error_out_of_date_khr";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "vk_error_incompatible_display_khr";
		case VK_ERROR_VALIDATION_FAILED_EXT:    return "vk_error_validation_failed_khr";
		case VK_ERROR_INVALID_SHADER_NV:        return "vk_error_invalid_shader_NV";
		case VK_ERROR_FRAGMENTATION_EXT:        return "vk_error_fragmentation_ext";
		case VK_ERROR_NOT_PERMITTED_EXT:        return "vk_error_not_permitted_ext";
		default:
			return "unknown vulkan error";
	}
}

/// messages for vuh specific errors
auto text(VuhError err)-> const char* {
	switch(err){
		case NoComputeCapableQueueFamilySpecified:   return "no compute capable queue family specified";
		case NoTransferCapableQueueFamilySpecified:  return "no transfer capable queue family specified";
		case NoSuitableMemoryFound:                  return "no suitable memory found";
		default:
			return "unknown vuh error";
	}
}

} // namespace vuh

