#include "instance.hpp"
#include "error.hpp"
#include "physicalDevice.hpp"
#include "utils.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_set>

using std::begin; using std::end;
#define ALL(c) begin(c), end(c)
#define ARR_VIEW(x) uint32_t(x.size()), x.data()

namespace vuh {
/// Default debug reporter used when user did not care to provide his own.
/// Logs stuff to std::cerr.
static auto VKAPI_ATTR default_logger(
      VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t
      , const char* layer_prefix
      , const char* message
      , void*       /*pUserData*/
      )-> VkBool32
{
   std::cerr << "[Vulkan]:" << layer_prefix << ": " << message << "\n";
   return VK_FALSE;
}

namespace {
	//
	static const auto default_layers = std::vector<const char*>{
#ifndef NDEBUG
	     "VK_LAYER_LUNARG_standard_validation",
	     "VK_LAYER_RENDERDOC_Capture",
#endif
	};
	//
	static const auto default_extensions = std::vector<const char*>{
#ifndef NDEBUG
	     VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
	     "VK_EXT_debug_utils",
#endif
	   "VK_KHR_device_group_creation"
	 , "VK_KHR_external_fence_capabilities"
	 , "VK_KHR_external_memory_capabilities"
	 , "VK_KHR_external_semaphore_capabilities"
};

//
auto properties_contain_name(const std::vector<VkLayerProperties>& properties, const char* name
                            )-> bool
{
	return std::find_if(ALL(properties)
	           , [name](const VkLayerProperties& p){ return strcmp(p.layerName, name) == 0;}
	       ) != end(properties);
}

//
auto extensions_contain_name(const std::vector<VkExtensionProperties>& extensions, const char* name
                             )-> bool
{
	return std::find_if(ALL(extensions)
	           , [name](const VkExtensionProperties e){ return strcmp(e.extensionName, name) == 0;}
	       ) != end(extensions);
}

//
auto contains(const std::vector<const char*>& values, const char* str)-> bool {
	return std::find_if(ALL(values), [str](const char* tst){ return 0 == strcmp(tst, str);})
	       != end(values);
}

//
auto add_default_layers(const std::vector<const char*>& layers)-> std::vector<const char*> {
	auto ret = layers;
	const auto available = available_layers();
	for(const auto& l: default_layers){
		if(properties_contain_name(available, l) && !contains(layers, l)) {
			ret.push_back(l);
		}
	}
	return ret;
}

//
auto add_default_extensions(const std::vector<const char*>& extensions)-> std::vector<const char*> {
	auto ret = extensions;
	const auto available = available_extensions();
	for(const auto& e: default_extensions){
		if(extensions_contain_name(available, e) && !contains(extensions, e)){
			ret.push_back(e);
		}
	}
	return ret;
}

/// Register a callback function for the extension VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
/// so that warnings emitted from the validation layer are actually printed.
auto registerLogger(VkInstance instance, logger_t logger)-> VkDebugReportCallbackEXT {
	auto ret = VkDebugReportCallbackEXT(nullptr);
	auto createInfo = VkDebugReportCallbackCreateInfoEXT{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT
	                   | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	createInfo.pfnCallback = logger;

	// explicitly load this function
	auto createFN = PFN_vkCreateDebugReportCallbackEXT(
	                              vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
	if(createFN){
		createFN(instance, &createInfo, nullptr, &ret);
	}
	return ret;
}

//
auto make_instance(const std::vector<const char*>& layers
                   , const std::vector<const char*>& extensions
                   , const VkApplicationInfo& app_info
                   )-> VkInstance
{
	auto ret = VkInstance{};
	auto extended_layers = add_default_layers(layers);
	VUH_CHECKOUT_RET(ret);
	auto extended_extensions = add_default_extensions(extensions);
	VUH_CHECKOUT_RET(ret);
	auto info = VkInstanceCreateInfo {
	              VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
	            , nullptr // pNext
	            , {} // flags
	            , &app_info
	            , ARR_VIEW(extended_layers)
	            , ARR_VIEW(extensions)
	};
	VUH_CHECK_RET(vkCreateInstance(&info, nullptr, &ret), ret);
	return ret;
}
} // namespace

/// doc me
Instance::Instance(const std::vector<const char*>& layers
                  , const std::vector<const char*>& extensions
                  , const VkApplicationInfo& app_info
                  , logger_t log_callback)
   : Instance(make_instance(layers, extensions, app_info))
{
	logger_attach(log_callback ? log_callback : default_logger);
}

///
Instance::~Instance() noexcept
{
	logger_release();
}

///
std::vector<PhysicalDevice> Instance::devices() const
{
	uint32_t num_devices = 0;
	vkEnumeratePhysicalDevices(*this, &num_devices, nullptr);
	auto ret = std::vector<PhysicalDevice>(num_devices);
	auto err = vkEnumeratePhysicalDevices( *this, &num_devices
	                                     , reinterpret_cast<VkPhysicalDevice*>(ret.data()));
	VUH_CHECK_RET(err, ret);
	return ret;
}

///
void Instance::logger_attach(logger_t logger)
{
	logger_release();
	_logger = logger;
	_reporter_cbk = registerLogger(*this, _logger);
}

///
auto Instance::log(const char* prefix, const char* message, VkDebugReportFlagsEXT flags) const-> void
{
	_logger(flags, VkDebugReportObjectTypeEXT{}, 0, 0, 0 , prefix, message, nullptr);
}

/// doc me
auto Instance::log(const std::string& prefix
                  , const std::string& message
                  , VkDebugReportFlagsEXT flags
                  ) const-> void
{
	log(prefix.c_str(), message.c_str(), flags);
}

///
auto Instance::logger_release() noexcept-> void
{
	if(_reporter_cbk){// unregister callback.
		auto destroyFn = PFN_vkDestroyDebugReportCallbackEXT(
		                    vkGetInstanceProcAddr(*this, "vkDestroyDebugReportCallbackEXT")
		                    );
		if(destroyFn){
			destroyFn(*this, _reporter_cbk, nullptr);
		}
	}
}

} // namespace vuh
