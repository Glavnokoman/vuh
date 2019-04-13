#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>

namespace vuh {

///
class Error: std::runtime_error {
public:
	using std::runtime_error::runtime_error;
	explicit Error(VkResult err): std::runtime_error("TBD"), _err(err) {}
private:
	VkResult _err;
}; //

///
inline auto VUH_CHECK(VkResult err) {
	if(err != VK_SUCCESS){
		throw Error(err);
	}
}

///
template<class T>
inline auto VUH_CHECK_RET(VkResult err, T) {
	if(err != VK_SUCCESS){
		throw Error(err);
	}
}

///
inline auto VUH_CHECKOUT(){}

///
template<class T>
inline auto VUH_CHECKOUT_RET(T){}
} // namespace vuh
