#include "vuh/instance.h"
#include "vuh/device.h"

#include <algorithm>
#include <array>
#include <iostream>

using std::begin; using std::end;
#define ALL(c) begin(c), end(c)
#define ARR_VIEW(x) uint32_t(x.size()), x.data()

namespace {
#ifndef NDEBUG
	static const std::array<const char*, 1> default_layers = {"VK_LAYER_LUNARG_standard_validation"};
	static const std::array<const char*, 1> default_extensions = {VK_EXT_DEBUG_REPORT_EXTENSION_NAME};
#else
	static const std::array<const char*, 0> default_layers = {};
	static const std::array<const char*, 0> default_extensions = {};
#endif

	/// @return true if value x can be extracted from an array with a given function
	template<class U, class F>
	auto contains(const char* x, const std::vector<U>& array, F&& fun)-> bool {
		return end(array) != std::find_if(ALL(array), [&](auto& r){return 0 == std::strcmp(x, fun(r));});
	}

	/// Extend vector of string literals by candidate values that have a macth in a reference set.
	template<class U, class T, class F>
	auto filter_list(std::vector<const char*> old_values ///< array to extend
	                 , const U& tst_values               ///< candidate values
	                 , const T& ref_values               ///< reference values
	                 , F&& ffield                        ///< maps reference values to candidate values manifold
	                 , vuh::debug_reporter_t report_cbk=nullptr ///< error reporter
	                 , const char* layer_msg=nullptr     ///< base part of the log message about unsuccessful candidate value
	                 )-> std::vector<const char*>
	{
		using std::begin;
		for(const auto& l: tst_values){
			if(contains(l, ref_values, ffield)){
				old_values.push_back(l);
			} else {
				if(report_cbk != nullptr){
					auto msg = std::string("value ") + l + " is missing";
					report_cbk({}, {}, 0, 0, 0, layer_msg, msg.c_str(), nullptr);
				}
			}
		}
		return old_values;
	}

	/// Filter requested layers, throw away those not present on particular instance.
	/// Add default validation layers to debug build.
	auto filter_layers(const std::vector<const char*>& layers) {
		const auto avail_layers = vk::enumerateInstanceLayerProperties();
		auto r = filter_list({}, layers, avail_layers
		                     , [](const auto& l){return l.layerName;});
		r = filter_list(std::move(r), default_layers, avail_layers
		                , [](const auto& l){return l.layerName;});
		return r;
	}

	/// Filter requested extensions, throw away those not present on particular instance.
	/// Add default debug extensions to debug build.
	auto filter_extensions(const std::vector<const char*>& extensions) {
		const auto avail_extensions = vk::enumerateInstanceExtensionProperties();
		auto r = filter_list({}, extensions, avail_extensions
		                     , [](const auto& l){return l.extensionName;});
		r = filter_list(std::move(r), default_extensions, avail_extensions
		                , [](const auto& l){return l.extensionName;});
		return r;
	}

	/// Default debug reporter used when user did not care to provide his own.
	static auto VKAPI_ATTR debugReporter(
	      VkDebugReportFlagsEXT , VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t
	      , const char*                pLayerPrefix
	      , const char*                pMessage
	      , void*                      /*pUserData*/
	      )-> VkBool32
	{
	   std::cerr << "[Vulkan]:" << pLayerPrefix << ": " << pMessage << "\n";
	   return VK_FALSE;
	}

	/// Create vulkan Instance with app specific parameters.
	auto createInstance(const std::vector<const char*> layers
	                   , const std::vector<const char*> extensions
	                   , const vk::ApplicationInfo& info
	                   )-> vk::Instance
	{
		auto createInfo = vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &info
		                                         , ARR_VIEW(layers), ARR_VIEW(extensions));
		return vk::createInstance(createInfo);
	}

	/// Register a callback function for the extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	/// so that warnings emitted from the validation layer are actually printed.
	auto registerReporter(vk::Instance instance, vuh::debug_reporter_t reporter
	                     )-> VkDebugReportCallbackEXT
	{
		auto ret = VkDebugReportCallbackEXT(nullptr);
		auto createInfo = VkDebugReportCallbackCreateInfoEXT{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT
		                   | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		createInfo.pfnCallback = reporter;

		// explicitly load this function
		auto createFN = PFN_vkCreateDebugReportCallbackEXT(
		                                  instance.getProcAddr("vkCreateDebugReportCallbackEXT"));
		if(createFN){
			createFN(instance, &createInfo, nullptr, &ret);
		}
		return ret;
	}
} // namespace

namespace vuh {
	/// Creates Instance object.
	/// In debug build in addition to user-defined layers attempts to load validation layers.
	Instance::Instance(const std::vector<const char*>& layers
	                   , const std::vector<const char*>& extension
	                   , const vk::ApplicationInfo& info
	                   , debug_reporter_t report_callback
	                   )
	   : _instance(createInstance(filter_layers(layers), filter_extensions(extension), info))
	   , _reporter(report_callback ? report_callback : debugReporter)
	   , _reporter_cbk(registerReporter(_instance, _reporter))
	{}

	/// Clean instance resources.
	Instance::~Instance() noexcept {
		clear();
	}

	/// Move constructor
	Instance::Instance(Instance&& o) noexcept
	   : _instance(o._instance)
	   , _reporter(o._reporter)
	   , _reporter_cbk(o._reporter_cbk)
	{
		o._instance = nullptr;
	}

	/// Move assignment
	auto Instance::operator=(Instance&& o) noexcept-> Instance& {
		using std::swap;
		swap(_instance, o._instance);
		swap(_reporter, o._reporter);
		swap(_reporter_cbk, o._reporter_cbk);
		return *this;
	}

	/// Destroy underlying vulkan instance.
	/// All resources associated with it, should be released before that.
	auto Instance::clear() noexcept-> void {
		if(_instance){
			if(_reporter_cbk){// unregister callback.
				auto destroyFn = PFN_vkDestroyDebugReportCallbackEXT(
				                    vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT")
				                    );
				if(destroyFn){
					destroyFn(_instance, _reporter_cbk, nullptr);
				}
			}

			_instance.destroy();
		}
	}

	/// @return vector of available vulkan devices
	auto Instance::devices()-> std::vector<Device> {
		auto physdevs = _instance.enumeratePhysicalDevices();
		auto r = std::vector<Device>{};
		for(auto pd: physdevs){
			r.emplace_back(*this, pd);
		}
		return r;
	}

	/// Log message using the reporter callback registered with the Vulkan instance.
	/// Default callback sends all messages to std::cerr
	auto Instance::report(const char* prefix    ///< prefix part of message. may contain component name, etc.
	                      , const char* message ///< message itself
	                      , VkDebugReportFlagsEXT flags ///< flags indicating message severity
	                      ) const-> void
	{
		_reporter(flags, VkDebugReportObjectTypeEXT{}, 0, 0, 0 , prefix, message, nullptr);
	}
} // namespace vuh
