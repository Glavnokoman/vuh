#pragma once

#include "vuh/mem/memProperties.h"
#include "arr/arrayIter.hpp"
#include "arr/arrayView.hpp"
#include "arr/copy_async.hpp"
#include "arr/deviceArray.hpp"
#include "arr/hostArray.hpp"
#include "mem/basicMemory.hpp"

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
struct ArrayClass<vuh::mem::properties::Device>{
   template<class T, class Alloc> using type = arr::DeviceArray<T, Alloc>;
};

/// Explicit trait specialization mapping ArrayClass<arr::properties::DeviceOnly> -> arr::DeviceOnlyArray
template<>
struct ArrayClass<vuh::mem::properties::DeviceOnly>{
   template<class T, class Alloc> using type = arr::DeviceOnlyArray<T, Alloc>;
};
} // namespace detail

/// Maps Array classes with different data exchange interfaces, to a single template type.
/// This enables std::vector-like type declarations of Arrays with different allocators.
/// Although in this case resulting classes have different data exchange interfaces,
/// this may still be useful since they all have similar binding properties to shaders.
template<class T, class Alloc=vuh::mem::AllocDevice<vuh::mem::properties::Device>>
using Array = typename detail::ArrayClass<typename Alloc::properties_t>::template type<T, Alloc>;

} // namespace vuh
