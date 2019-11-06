#pragma once

#include "vuh/mem/memProperties.h"
#include "imageIter.hpp"
#include "vuh/mem/allocDevice.hpp"
#include "basicImage.hpp"
#include <vuh/traits.hpp>
#include <algorithm>
#include <cassert>

namespace vuh {
    namespace img {

        /// Image with host data exchange interface suitable for memory allocated in device-local space.
        /// Memory allocation and underlying buffer creation is managed by allocator defined by a template parameter.
        /// Such allocator is expected to allocate memory in device local memory not mappable
        /// for host access. However actual allocation may take place in a host-visible memory.
        /// Some functions (like toHost(), fromHost()) switch to using the simplified data exchange methods
        /// in that case. Some do not. In case all memory is host-visible (like on integrated GPUs) using this class
        /// may result in performance penalty.
        template<class T, class Alloc>
        class Device2DImage: public Basic2DImage<T, Alloc>{
            using Base = Basic2DImage<T, Alloc>;
        public:
            using value_type = T;
            /// Create an instance of DeviceArray with given number of elements. Memory is uninitialized.
            Device2DImage(const vuh::Device& dev   ///< device to create array on
                    , const size_t width     ///< width of image
                    , const size_t height     ///< height of image
                    , const vhn::Format fmt=vhn::Format::eR8G8B8A8Unorm /// format
                    , const vhn::MemoryPropertyFlags flags_mem={} ///< additional (to defined by allocator) memory usage flags
                    , const vhn::ImageUsageFlags flags_image={})   ///< additional (to defined by allocator) buffer usage flags
                    : Base(dev, width, height, fmt, flags_mem, flags_image){}

            /// Create an instance of Device2DImage and initialize memory by content of vuh::Array host iterable.
            Device2DImage(const vuh::Device& dev  ///< device to create array on
                    , const vuh::Array<T>& arr          ///< iterable to initialize from
                    , const size_t width     ///< width of image
                    , const vhn::Format fmt=vhn::Format::eR8G8B8A8Unorm /// format
                    , const vhn::MemoryPropertyFlags flags_mem={} ///< additional (to defined by allocator) memory usage flags
                    , const vhn::ImageUsageFlags flags_image={})	  ///< additional (to defined by allocator) buffer usage flags
                    : Base(dev, width, arr.size() / width + 1, fmt, flags_mem, flags_image) {
                fromArray(arr);
            }

            /// Copy data from vuh::Array to image memory.
            auto fromArray(const vuh::Array<T>& arr)-> void {
                const vhn::Buffer& buffer = const_cast<vuh::Array<T>&>(arr).buffer();
                arr::copyBufferToImage(Base::_dev, buffer, *this, Base::width(), Base::height());
            }

            /// @return copy image to vuh::Array data.
            auto toHost() const-> vuh::Array<T> {
                vuh::Array<T> arr(Base::_dev, Base::size());
                arr::copyImageToBuffer(Base::_dev, *this, arr, Base::width(), Base::height());
                return arr;
            }
        }; // class Device2DImage
    } // namespace img
} // namespace vuh
