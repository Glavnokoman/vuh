#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

namespace vuh {
	class Device;

	using debug_reporter_t = PFN_vkDebugReportCallbackEXT;

	/// Working with vuh starts from creating an object of this class.
	/// Its main responsibility is listing the available devices and handling extensions and layers.
	/// It is also responsible for logging vuh and vulkan messages.
	/// In debug builds adds default validation layer/extension.
	/// Default debug reporter sends messages to std::cerr.
	/// Reentrant.
	class Instance {
	public:
		explicit Instance(const std::vector<const char*>& layers={}
		                 , const std::vector<const char*>& extension={}
		                 , const vk::ApplicationInfo& info={nullptr, 0, nullptr, 0, VK_API_VERSION_1_0}
		                 , debug_reporter_t report_callback=nullptr
		                 );

		~Instance() noexcept;

		Instance(const Instance&) = delete;
		auto operator= (const Instance&)-> Instance& = delete;
		Instance(Instance&&) noexcept;
		auto operator= (Instance&&) noexcept-> Instance&;

		auto devices()-> std::vector<vuh::Device>;
		auto report(const char* prefix, const char* message
		            , VkDebugReportFlagsEXT flags=VK_DEBUG_REPORT_INFORMATION_BIT_EXT) const-> void;
	private: // helpers
		auto clear() noexcept-> void;
	private: // data
		vk::Instance _instance;             ///< vulkan instance
		VkDebugReportCallbackEXT _reporter; ///< report callback to handle messages sent by validation layers
		std::vector<const char*> _layers;   ///< some retarded drivers still require the device layers to be explicitely set, hence this variable.
	}; // class Instance
} // namespace vuh
