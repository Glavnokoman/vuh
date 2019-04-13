#include "instance.hpp"
#include "error.hpp"
#include "utils.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>
#include <unordered_set>

using std::begin; using std::end;
#define ALL(c) begin(c), end(c)
#define ARR_VIEW(x) uint32_t(x.size()), x.data()

namespace vuh {
namespace {
#ifndef NDEBUG
	static const auto default_layers = std::vector<const char*>{"VK_LAYER_LUNARG_standard_validation"};
	static const auto default_extensions = std::vector<const char*>{VK_EXT_DEBUG_REPORT_EXTENSION_NAME};
#else
	static const auto default_layers = std::vector<const char*>{};
	static const auto default_extensions = std::vector<const char*>{};
#endif

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
Instance::Instance( const std::vector<const char*>& layers
                  , const std::vector<const char*>& extensions
                  , const VkApplicationInfo& app_info
                  )
   : Instance(make_instance(layers, extensions, app_info))
{}

} // namespace vuh
