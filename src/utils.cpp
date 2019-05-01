#include "utils.hpp"
#include "error.hpp"

namespace vuh {

/// @return available layers
auto available_layers()-> std::vector<VkLayerProperties> {
	auto n_available = uint32_t(0);
	vkEnumerateInstanceLayerProperties(&n_available, nullptr);

	auto ret = std::vector<VkLayerProperties>(n_available);
	VUH_CHECK_RET( vkEnumerateInstanceLayerProperties(&n_available, ret.data())
	             , ret);
	return ret;
}

/// @return Extensions available from the given layer.
/// If layer name paramerter is nullptr extensions provided by the Vulkan implementation or
/// implicitely enabled layers are returned.
auto available_extensions(const char* layerName)-> std::vector<VkExtensionProperties> {
	auto n_available = uint32_t(0);
	vkEnumerateInstanceExtensionProperties(layerName, &n_available, nullptr);

	auto ret = std::vector<VkExtensionProperties>(n_available);
	VUH_CHECK_RET( vkEnumerateInstanceExtensionProperties(layerName, &n_available, ret.data())
	             , ret);
	return ret;
}

/// @return All available extensions.
auto available_extensions()-> std::vector<VkExtensionProperties> {
	auto ret = available_extensions(nullptr);
	VUH_CHECKOUT_RET(ret);
	for(auto p: available_layers()){
		auto layer_extensions = available_extensions(p.layerName);
		VUH_CHECKOUT_RET(ret);
		ret.insert(end(ret), begin(layer_extensions), end(layer_extensions));
	}
	return ret;
}

/// Find memory id-s matching desired properties and requirements.
/// @return array of suitable memory indicies
auto selectMemory( VkPhysicalDevice device
                 , VkMemoryRequirements requirements
                 , VkMemoryPropertyFlags properties
                 )-> std::vector<std::uint32_t>
{
	auto ret = std::vector<std::uint32_t>{};
	auto mem_available = VkPhysicalDeviceMemoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(device, &mem_available);
	for(uint32_t i = 0; i < mem_available.memoryTypeCount; ++i){
		if( (requirements.memoryTypeBits & (1u << i))
		    && ((properties & mem_available.memoryTypes[i].propertyFlags) == properties))
		{
			ret.push_back(i);
		}
	}
	return ret;
}

} // namespace vuh
