#pragma once

#include "common.hpp"

#include <string>
#include <vector>

namespace vuh {
using logger_t = PFN_vkDebugReportCallbackEXT;

struct PhysicalDevice;

/// doc me
class Instance: public detail::Resource<VkInstance, vkDestroyInstance> {
public:
	using Base::Resource;
	explicit Instance( const std::vector<const char*>& layers={}
	                 , const std::vector<const char*>& extensions={}
	                 , const VkApplicationInfo& app_info={}
	                 , logger_t log_callback=nullptr);

	~Instance() noexcept;

	auto devices() const->std::vector<PhysicalDevice>;
	auto logger_attach(logger_t logger)-> void;
	auto log( const char* prefix, const char* message
	        , VkDebugReportFlagsEXT flags=VK_DEBUG_REPORT_INFORMATION_BIT_EXT) const-> void;
	auto log(const std::string& prefix, const std::string& message
	         , VkDebugReportFlagsEXT flags=VK_DEBUG_REPORT_INFORMATION_BIT_EXT) const-> void;
private: // helpers
	auto logger_release() noexcept-> void;
private: // data
	logger_t _logger;                               ///< points to actual reporting function. This pointer is registered with a reporter callback but can also be used directly.
	VkDebugReportCallbackEXT _reporter_cbk=nullptr; ///< report callback. Only used to release the handle in the end.
}; // class Instance
} // namespace vuh
