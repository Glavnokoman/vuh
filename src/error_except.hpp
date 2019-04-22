#pragma once

#include "error_codes.hpp"

#include <vulkan/vulkan_core.h>

#include <stdexcept>

namespace vuh {

///
class Error: std::runtime_error {
public:
	using std::runtime_error::runtime_error;
	explicit Error(VkResult err): std::runtime_error(error::text(err)){}
}; //

///
inline auto VUH_CHECK(VkResult err) {
	if(err != VK_SUCCESS){
		throw Error(err);
	}
}

///
template<class T>
inline auto VUH_CHECK_RET(VkResult err, const T&) {
	if(err != VK_SUCCESS){
		throw Error(err);
	}
}

///
inline auto VUH_CHECKOUT(){}

///
template<class T>
inline auto VUH_CHECKOUT_RET(T){}

/// signal vuh specific error
inline auto VUH_SIGNAL(std::int32_t err) {

}

} // namespace vuh
