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
        class BasicDeviceImage2D: public BasicImage2D<T, Alloc> {
            using Base = BasicImage2D<T, Alloc>;
        public:
            using value_type = T;
            /// Create an instance of BasicDeviceImage2D with given number of elements. Memory is uninitialized.
            BasicDeviceImage2D(const vuh::Device& dev   ///< device to create array on
                    , const size_t imW     ///< width of image
                    , const size_t imH     ///< height of image
                    , const vhn::Format imFmt = vhn::Format::eR8G8B8A8Unorm /// format
                    , const vhn::DescriptorType imDesc = vhn::DescriptorType::eStorageImage
                    , const vhn::ImageUsageFlags imF = {})   ///< additional (to defined by allocator) buffer usage flags
                    : Base(dev, imW, imH, imFmt, imDesc, imF){}

            /// Create an instance of BasicDeviceImage2D and initialize memory by content of vuh::Array host iterable.
            BasicDeviceImage2D(const vuh::Device& dev  ///< device to create array on
                    , const vuh::Array<T>& arr          ///< iterable to initialize from
                    , const size_t imW     ///< width of image
                    , const vhn::Format imFmt = vhn::Format::eR8G8B8A8Unorm /// format
                    , const vhn::DescriptorType imDesc = vhn::DescriptorType::eStorageImage
                    , const vhn::ImageUsageFlags imFlags = {})	  ///< additional (to defined by allocator) buffer usage flags
                    : Base(dev, imW, arr.size() / imW + 1, imFmt, imDesc, imFlags) {
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
        }; // class BasicDeviceImage2D

        template<class T, class Alloc>
        class DeviceImage2D: public BasicDeviceImage2D<T, Alloc> {
            using Base = BasicDeviceImage2D<T, Alloc>;
        public:
            using value_type = T;

            /// Create an instance of DeviceImage2D with given number of elements. Memory is uninitialized.
            DeviceImage2D(const vuh::Device &dev   ///< device to create array on
                    , const size_t imW     ///< width of image
                    , const size_t imH     ///< height of image
                    , const vhn::Format imFmt = vhn::Format::eR8G8B8A8Unorm /// format
                    , const vhn::ImageUsageFlags imF = {})   ///< additional (to defined by allocator) buffer usage flags
                    : Base(dev, imW, imH, imFmt, vhn::DescriptorType::eStorageImage,
                           imF) {}

            /// Create an instance of DeviceImage2D and initialize memory by content of vuh::Array host iterable.
            DeviceImage2D(const vuh::Device &dev  ///< device to create array on
                    , const vuh::Array<T> &arr          ///< iterable to initialize from
                    , const size_t imW     ///< width of image
                    , const vhn::Format imFmt = vhn::Format::eR8G8B8A8Unorm /// format
                    , const vhn::ImageUsageFlags imF = {})      ///< additional (to defined by allocator) buffer usage flags
                    : Base(dev, imW, arr.size() / imW + 1, imFmt,
                           vhn::DescriptorType::eStorageImage, imF) {
            }
        };

        template<class T, class Alloc>
        class DeviceCombinedImage2D: public BasicDeviceImage2D<T, Alloc> {
            using Base = BasicDeviceImage2D<T, Alloc>;
        public:
            using value_type = T;

            /// Create an instance of DeviceCombinedImage2D with given number of elements. Memory is uninitialized.
            DeviceCombinedImage2D(const vuh::Device &dev   ///< device to create array on
                    , const size_t imW     ///< width of image
                    , const size_t imH     ///< height of image
                    , const vhn::Format imFmt = vhn::Format::eR8G8B8A8Unorm /// format
                    , const vhn::ImageUsageFlags imF = {})   ///< additional (to defined by allocator) buffer usage flags
                    : Base(dev, imW, imH, imFmt, vhn::DescriptorType::eCombinedImageSampler,
                           imF | vhn::ImageUsageFlagBits::eSampled) {}

            /// Create an instance of DeviceCombinedImage2D and initialize memory by content of vuh::Array host iterable.
            DeviceCombinedImage2D(const vuh::Device &dev  ///< device to create array on
                    , const vuh::Array<T> &arr          ///< iterable to initialize from
                    , const size_t imW     ///< width of image
                    , const vhn::Format imFmt = vhn::Format::eR8G8B8A8Unorm /// format
                    , const vhn::ImageUsageFlags imF = {})      ///< additional (to defined by allocator) buffer usage flags
                    : Base(dev, arr, imW, imFmt,
                           vhn::DescriptorType::eCombinedImageSampler, imF | vhn::ImageUsageFlagBits::eSampled) {
            }
        };

    } // namespace img
} // namespace vuh
