#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {
	class Device;

	using debug_reporter_t = PFN_vkDebugReportCallbackEXT;

	/// Mostly RAII wrapper over Vulkan instance.
	/// Defines layers and extensions in use and can enumerate present compute-capable devices.
	class Instance {
	public:
		explicit Instance(const std::vector<const char*>& layers={}
		                 , const std::vector<const char*>& extension={}
		                 , const vk::ApplicationInfo& info={nullptr, 0, nullptr, 0, VK_API_VERSION_1_0}
		                 , debug_reporter_t report_callback=nullptr
		                 );

		~Instance() noexcept;

		Instance(const Instance&) = delete;
		Instance& operator= (const Instance&) = delete;
		Instance(Instance&&) = default;
		Instance& operator= (Instance&&) = default;

		auto devices()-> std::vector<vuh::Device>;
	protected: // data
		vk::Instance _instance;             ///< vulkan instance
		VkDebugReportCallbackEXT _reporter; ///< report callback to handle messages sent by validation layers
		std::vector<const char*> _layers;   ///< some retarded drivers still require the device layers to be explicitely set, hence this variable.
	}; // class Instance
} // namespace vuh
