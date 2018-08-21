#pragma once

#include "arr/arrayProperties.h"
#include "arr/arrayIter.hpp"
#include "arr/arrayView.hpp"
#include "arr/copy_async.hpp"
#include "arr/deviceArray.hpp"
#include "arr/hostArray.hpp"

namespace vuh {
namespace detail {

/// Traits to associate array properties to underlying Array classes (HostArray, DeviceArray).
/// Default mapping is ArrayClass<T> -> arr::HostArray.
template<class Props>
struct ArrayClass {
	template<class T, class Alloc> using type = arr::HostArray<T, Alloc>;
};

/// Explicit trait specialization mapping ArrayClass<arr::properties::Device> -> arr::DeviceArray
template<>
struct ArrayClass<arr::properties::Device>{
   template<class T, class Alloc> using type = arr::DeviceArray<T, Alloc>;
};

/// Explicit trait specialization mapping ArrayClass<arr::properties::DeviceOnly> -> arr::DeviceOnlyArray
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
	using HostCached = arr::AllocDevice<arr::properties::HostCached>;
	using HostCoherent = arr::AllocDevice<arr::properties::HostCoherent>;
} // namespace mem

/// Maps Array classes with different data exchange interfaces, to a single templated type.
/// This enables std::vector-like type declarations of Arrays with different allocators.
/// Althogh in this case resulting classes have different data exchange interfaces,
/// this may still be useful since they all have similae binding properties to shaders.
template<class T, class Alloc=arr::AllocDevice<arr::properties::Device>>
using Array = typename detail::ArrayClass<typename Alloc::properties_t>::template type<T, Alloc>;

} // namespace vuh
