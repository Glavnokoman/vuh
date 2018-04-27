#pragma once

#include <vuh/string_view.h>

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {
	class Device;

	/// Mostly RAII wrapper over Vulkan instance.
	/// Instance of the vuh::Instance defines layers and extensions in use
	/// and can enumerate present devices.
	class Instance {
	public:
		explicit Instance( const std::vector<string_view>& layers={}
		                 , const std::vector<string_view>& extension={}
		                 , vk::ApplicationInfo info={nullptr, 0, nullptr, 0, VK_API_VERSION_1_0}
		                 );

		~Instance() noexcept;

		Instance(const Instance&) = delete;
		Instance& operator= (const Instance&) = delete;
		Instance(Instance&&) = default;
		Instance& operator= (Instance&&) = default;

		auto devices()-> std::vector<vuh::Device>;
		auto resetMessageHandler(VkDebugReportCallbackEXT)-> void;
	protected: // data
		vk::Instance _instance;               ///< vulkan instance
		VkDebugReportCallbackEXT _report_cbk; ///< report callback to handle messages sent by validation layers
	}; // class Instance
} // namespace vuh
