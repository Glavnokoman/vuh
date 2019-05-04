#pragma once

#include <vulkan/vulkan_core.h>

#include <type_traits>

namespace vuh::detail {

template<class T> using Deleter = void(T, const VkAllocationCallbacks*);
template<class T> using DeleterPtr = Deleter<T>*;
template<class T> using DeviceDeleter = void(VkDevice, T, const VkAllocationCallbacks*);
template<class T> using DeviceDeleterPtr = DeviceDeleter<T>*;

///
template<class T, DeleterPtr<T> D>
class Resource {
protected:
	using Base = Resource;
public:
	explicit Resource(T resource) noexcept:_base_resource{resource} {}
	~Resource() noexcept { D(_base_resource, nullptr); }

	Resource(Resource&& other) noexcept
	   : _base_resource{other._base_resource}
	{
		other._base_resource = nullptr;
	}
	auto operator= (Resource&& other) noexcept-> Resource& {
		std::swap(other._base_resource, _base_resource);
		return *this;
	}

	operator T() const {return _base_resource;}
protected: // data
	T _base_resource; ///< base resource
}; // class Resource
} // namespace vuh::detail
