#pragma once

#include "allocatorTraits.hpp"
#include "bufferDevice.hpp"
#include "bufferDeviceImpl.hpp"
#include "bufferHost.hpp"
#include "bufferSpan.hpp"

namespace vuh {
namespace detail {
/// Traits to associate array properties to underlying Array classes (BufferHost, BufferDevice).
/// Default mapping is BufferType<T> -> BufferHost.
template<class Traits>
struct BufferType {
	template<class T, class Alloc> using type = BufferHost<T, Alloc>;
};

/// Explicit trait specialization mapping BufferType<allocator::traits::Device> -> BufferDevice
template<>
struct BufferType<allocator::traits::Device>{
	template<class T, class Alloc> using type = BufferDevice<T, Alloc>;
};

/// Explicit trait specialization mapping BufferType<allocator::traits::DeviceOnly> -> BufferDeviceOnly
template<>
struct BufferType<allocator::traits::DeviceOnly>{
   template<class T, class Alloc> using type = BufferDeviceOnly<T, Alloc>;
};
} // namespace detail

/// defines some shortcut allocators types
namespace mem {
	using DeviceOnly = AllocatorDevice<allocator::traits::DeviceOnly>;
	using Device = AllocatorDevice<allocator::traits::Device>;
	using Unified = AllocatorDevice<allocator::traits::Unified>;
	using Host = AllocatorDevice<allocator::traits::Host>;
	using HostCached = AllocatorDevice<allocator::traits::HostCached>;
	using HostCoherent = AllocatorDevice<allocator::traits::HostCoherent>;
} // namespace mem

/// Maps Array classes with different data exchange interfaces, to a single templated type.
/// This enables std::vector-like type declarations of Arrays with different allocators.
/// Althogh in this case resulting classes have different data exchange interfaces,
/// this may still be useful since they all have similae binding properties to shaders.
template<class T, class Alloc=AllocatorDevice<allocator::traits::Device>>
using Buffer = typename detail::BufferType<typename Alloc::properties_t>::template type<T, Alloc>;

} // namespace vuh
