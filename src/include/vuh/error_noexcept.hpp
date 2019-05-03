#pragma once
#ifdef VUH_NO_EXCEPTIONS

#include <vulkan/vulkan.h>

#define VUH_CHECKOUT() do {if(vuh::error::err()){ return; }} while(false)
#define VUH_CHECKOUT_RET(r) do {if(vuh::error::err()){ return r; }} while(false)
#define VUH_CHECK(e) do {const auto vuh__err = e; if(vuh__err != VK_SUCCESS) {vuh::error::set(vuh__err); return;}} while(false)
#define VUH_CHECK_RET(e, r) do {const auto vuh__err = e; if(vuh__err != VK_SUCCESS) {vuh::error::set(vuh__err); return r;}} while(false)

namespace vuh::error {
	auto set(VkResult err)-> void;
	auto err()-> bool;
	auto last()-> VkResult;
} // vuh::error

#endif // VUH_NO_EXCEPTIONS
