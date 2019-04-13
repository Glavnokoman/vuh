#pragma once

#include "common.hpp"

#include <vector>

namespace vuh {
/// doc me
class Instance: public detail::Resource<VkInstance, vkDestroyInstance>
{
public:
	using Base::Resource;
	explicit Instance( const std::vector<const char*>& layers={}
	                 , const std::vector<const char*>& extensions={}
	                 , const VkApplicationInfo& app_info={});
private: // data
}; // class Instance
} // namespace vuh
