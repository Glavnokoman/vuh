#include "vuh/instance.h"
#include "vuh/device.h"

#include <algorithm>
#include <iostream>

using std::begin; using std::end;
#define ALL(c) begin(c), end(c)
#define ARR_VIEW(x) uint32_t(x.size()), x.data()
#define ST_VIEW(s)  uint32_t(sizeof(s)), &s

namespace {
	static const char* debug_layers[] = {"VK_LAYER_LUNARG_standard_validation"};
	static const char* debug_extensions[] = {VK_EXT_DEBUG_REPORT_EXTENSION_NAME};

	//
	template<class U, class F>
	auto contains(const char* x, const std::vector<U>& array, F&& fun)-> bool {
		return end(array) != std::find_if(ALL(array), [&](auto& r){return 0 == std::strcmp(x, fun(r));});
	}

	//
	template<class U, class T, class F>
	auto filter_list(std::vector<const char*> old_values
	                 , const U& tst_values, const T& ref_values, F&& ffield
	                 , vuh::debug_reporter_t report_cbk=nullptr
	                 , const char*  layer_msg=nullptr
	                 )-> std::vector<const char*> {
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

	// Filter requested layers, throw away those not present on particular instance.
	// Add default validation layers to debug build.
	auto filter_layers(const std::vector<const char*>& layers) {
		const auto avail_layers = vk::enumerateInstanceLayerProperties();
		auto r = filter_list({}, layers, avail_layers
		                     , [](const auto& l){return l.layerName;});
		#ifndef NDEBUG
		r = filter_list(std::move(r), debug_layers, avail_layers
		                , [](const auto& l){return l.layerName;});
		#endif //NDEBUG
		return r;
	}

	// Filter requested extensions, throw away those not present on particular instance.
	// Add default debug extensions to debug build.
	auto filter_extensions(const std::vector<const char*>& extensions) {
		const auto avail_extensions = vk::enumerateInstanceExtensionProperties();
		auto r = filter_list({}, extensions, avail_extensions
		                     , [](const auto& l){return l.extensionName;});
		#ifndef NDEBUG
		r = filter_list(std::move(r), debug_extensions, avail_extensions
		                , [](const auto& l){return l.extensionName;});
		#endif //NDEBUG
		return r;
	}

	// Default debug reporter used when user did not care to provide his own.
	static auto debugReporter(
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
		} else {
			std::cerr << "Could not load vkCreateDebugReportCallbackEXT\n";
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
	   , _reporter(registerReporter(_instance, report_callback ? report_callback : debugReporter))
	   , _layers(filter_layers(layers)) // @todo: fix layers filtered twice.
	{
	}

	/// Clean instance resources.
	Instance::~Instance() noexcept {
		if(_reporter){// unregister callback.
			auto destroyFn = PFN_vkDestroyDebugReportCallbackEXT(
			                     vkGetInstanceProcAddr(_instance, "vkDestroyDebugReportCallbackEXT"));
			if(destroyFn){
				destroyFn(_instance, _reporter, nullptr);
			}
		}

		_instance.destroy();
	}

	/// list of local compute-capable devices
	auto Instance::devices()-> std::vector<Device>
	{
		throw "not implemented";
	}
} // namespace vuh
