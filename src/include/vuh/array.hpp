#pragma once

#include "arr/arrayProperties.h"
#include "arr/deviceArray.hpp"
#include "arr/hostArray.hpp"

namespace vuh {
namespace detail {

///
template<class Props>
struct ArrayClass {
	template<class T, class Alloc> using type = arr::HostArray<T, Alloc>;
};

///
template<>
struct ArrayClass<arr::properties::Device>{
   template<class T, class Alloc> using type = arr::DeviceArray<T, Alloc>;
};

///
template<>
struct ArrayClass<arr::properties::DeviceOnly>{
   template<class T, class Alloc> using type = arr::DeviceOnlyArray<T, Alloc>;
};
} // namespace detail

/// defines some shortcut allocators types
namespace mem {
	using DeviceOnly = arr::AllocDevice<arr::properties::DeviceOnly>;
	using Device = arr::AllocDevice<arr::properties::Device>;
	using Unified = arr::AllocDevice<arr::properties::Unified>;
	using Host = arr::AllocDevice<arr::properties::Host>;
} // namespace mem

///
template<class T, class Alloc=arr::AllocDevice<arr::properties::Device>>
using Array = typename detail::ArrayClass<typename Alloc::properties_t>::template type<T, Alloc>;

} // namespace vuh
