#pragma once

#include "vuh/core/core.hpp"
#include "vuh/core/vuhBasic.hpp"
#include <vector>

namespace vuh {
	class Device;

	using debug_reporter_t = PFN_vkDebugReportCallbackEXT;

	using debug_reporter_flags_t = VkDebugReportFlagsEXT;
	#define DEF_DBG_REPORT_FLAGS  (VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)

	/// Working with vuh starts from creating an object of this class.
	/// Its main responsibility is listing the available devices and handling extensions and layers.
	/// It is also responsible for logging vuh and vulkan messages.
	/// In debug builds adds default validation layer/extension.
	/// Default debug reporter sends messages to std::cerr.
	/// Reentrant.
	class Instance : public VuhBasic {
	public:
		explicit Instance(const std::vector<const char*>& layers={}
		                 , const std::vector<const char*>& extension={}
		                 , const vhn::ApplicationInfo& info={nullptr, 0, nullptr, 0, VK_API_VERSION_1_0}
		                 , debug_reporter_t report_callback=nullptr
		                 , debug_reporter_flags_t report_flags=DEF_DBG_REPORT_FLAGS
		                 , void* report_userdata=nullptr
		                 );

		~Instance() noexcept;

		Instance(const Instance&) = delete;
		auto operator= (const Instance&)-> Instance& = delete;
		Instance(Instance&&) noexcept;
		auto operator= (Instance&&) noexcept-> Instance&;

		auto devices()-> std::vector<vuh::Device>;
		auto report(const char* prefix, const char* message
		            , VkDebugReportFlagsEXT flags=VK_DEBUG_REPORT_INFORMATION_BIT_EXT) const-> void;

		VULKAN_HPP_TYPESAFE_EXPLICIT operator vhn::Instance() const { return _instance; }
		explicit operator bool() const;
		bool operator!() const;

    private: // helpers
		auto clear() noexcept-> void;
	private: // data
		vhn::Instance 				_instance;     ///< vulkan instance
		debug_reporter_t 			_reporter; ///< points to actual reporting function. This pointer is registered with a reporter callback but can also be used directly.
		VkDebugReportCallbackEXT 	_reporter_cbk; ///< report callback. Only used to release the handle in the end.
	}; // class Instance
} // namespace vuh
