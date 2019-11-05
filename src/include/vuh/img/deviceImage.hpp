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

        /// Image class not supposed to take part in data exchange with host.
        /// The only valid use for such arrays is passing them as (in or out) argument to a shader.
        /// Resources allocation is handled by allocator defined by a template parameter which is supposed to provide
        /// memory and underlying vulkan image with suitable flags.
        template<class T, class Alloc>
        class DeviceOnly2DImage: virtual public Basic2DImage<T, Alloc> {
        public:
            using value_type = T;
            /// Constructs object of the class on given device.
            /// Memory is left unintitialized.
            DeviceOnly2DImage( vuh::Device& dev  ///< deice to create array on
                    , size_t width     ///< width of image
                    , size_t height     ///< height of image
                    , vhn::Format format=vhn::Format::eR8G8B8A8Unorm/// format
                    , vhn::MemoryPropertyFlags flags_memory={} ///< additional (to defined by allocator) memory usage flags
                    , vhn::ImageUsageFlags flags_image={})   ///< additional (to defined by allocator) buffer usage flags
                    : Basic2DImage<T, Alloc>(dev, width, height, format, flags_memory, flags_image)
            {}
        }; // class DeviceOnly2DImage

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
            Device2DImage(vuh::Device& dev   ///< device to create array on
                    , size_t width     ///< width of image
                    , size_t height     ///< height of image
                    , vhn::Format fmt=vhn::Format::eR8G8B8A8Unorm /// format
                    , vhn::MemoryPropertyFlags flags_mem={} ///< additional (to defined by allocator) memory usage flags
                    , vhn::ImageUsageFlags flags_image={})   ///< additional (to defined by allocator) buffer usage flags
                    : Base(dev, width, height, fmt, flags_mem, flags_image)
            {}

            /// Create an instance of Device2DImage and initialize memory by content of some host iterable.
            template<class C, class=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>>
            Device2DImage(vuh::Device& dev  ///< device to create array on
                    , const C& c          ///< iterable to initialize from
                    , size_t width     ///< width of image
                    , vhn::Format fmt=vhn::Format::eR8G8B8A8Unorm /// format
                    , vhn::MemoryPropertyFlags flags_mem={} ///< additional (to defined by allocator) memory usage flags
                    , vhn::ImageUsageFlags flags_image={})	  ///< additional (to defined by allocator) buffer usage flags
                    : Base(dev, width, c.size() / width + 1, fmt, flags_mem, flags_image)
            {
                using std::begin; using std::end;
                fromHost(begin(c), end(c));
            }

            /// Copy data from host range to array memory.
            template<class It1, class It2>
            auto fromHost(It1 begin, It2 end)-> void {
                if(Base::isHostVisible()){
                    std::copy(begin, end, host_data());
                    Base::_dev.unmapMemory(Base::_mem);
                } else { // memory is not host visible, use staging buffer
                    auto stage_buf = vuh::arr::HostArray<T, vuh::mem::AllocDevice<vuh::mem::properties::HostCoherent>>(Base::_dev, begin, end);
                    copyBufferToImage(Base::_dev, stage_buf, *this, Base::width(), Base::height());
                }
            }

            /// Copy data from host range to array memory with offset
            template<class It1, class It2>
            auto fromHost(It1 begin, It2 end, size_t offset)-> void {
                if(Base::isHostVisible()){
                    std::copy(begin, end, host_data() + offset);
                    Base::_dev.unmapMemory(Base::_mem);
                } else { // memory is not host visible, use staging buffer
                    auto stage_buf = arr::HostArray<T, vuh::mem::AllocDevice<vuh::mem::properties::HostCoherent>>(Base::_dev, begin, end);
                    copyBufferToImage(Base::_dev, stage_buf, *this, Base::width(), Base::height(), offset*sizeof(T));
                }
            }

            /// Copy data from array memory to host location indicated by iterator.
            /// The whole array data is copied over.
            template<class It>
            auto toHost(It copy_to) const-> void {
                if(Base::isHostVisible()){
                    std::copy_n(host_data(), Base::size(), copy_to);
                    Base::_dev.unmapMemory(Base::_mem);
                } else {
                    using std::begin; using std::end;
                    auto stage_buf = vuh::arr::HostArray<T, vuh::mem::AllocDevice<vuh::mem::properties::HostCached>>(Base::_dev, Base::size());
                    copyImageToBuffer(Base::_dev, *this, stage_buf, Base::width(), Base::height());
                    std::copy(begin(stage_buf), end(stage_buf), copy_to);
                }
            }

            /// Copy-transform data from array memory to host location indicated by iterator.
            /// The whole array data is transformed.
            template<class It, class F>
            auto toHost(It copy_to, F&& fun) const-> void {
                if(Base::isHostVisible()){
                    auto copy_from = host_data();
                    std::transform(copy_from, copy_from + Base::size(), copy_to, std::forward<F>(fun));
                    Base::_dev.unmapMemory(Base::_mem);
                } else {
                    using std::begin; using std::end;
                    auto stage_buf = arr::HostArray<T, vuh::mem::AllocDevice<vuh::mem::properties::HostCached>>(Base::_dev, Base::size());
                    copyImageToBuffer(Base::_dev, *this, stage_buf, Base::width(), Base::height());
                    std::transform(begin(stage_buf), end(stage_buf), copy_to, std::forward<F>(fun));
                }
            }

            /// Copy-transform the chunk of data of given size from the beginning of array.
            template<class It, class F>
            auto toHost( It copy_to  ///< iterator indicating starting position to write to
                    , size_t size ///< number of elements to transform
                    , F&& fun     ///< transform function
            ) const-> void
            {
                if(Base::isHostVisible()){
                    auto copy_from = host_data();
                    std::transform(copy_from, copy_from + size, copy_to, std::forward<F>(fun));
                    Base::_dev.unmapMemory(Base::_mem);
                } else {
                    using std::begin; using std::end;
                    auto stage_buf = arr::HostArray<T, vuh::mem::AllocDevice<vuh::mem::properties::HostCached>>(Base::_dev, size);
                    copyImageToBuffer(Base::_dev, *this, stage_buf, Base::width(), Base::height());
                    std::transform(begin(stage_buf), end(stage_buf), copy_to, std::forward<F>(fun));
                }
            }

            /// Copy range of values from device to host memory.
            template<class DstIter>
            auto rangeToHost(size_t offset_begin, size_t offset_end, DstIter dst_begin) const-> void {
                if(Base::isHostVisible()){
                    auto copy_from = host_data();
                    std::copy(copy_from + offset_begin, copy_from + offset_end, dst_begin);
                    Base::_dev.unmapMemory(Base::_mem);
                } else {
                    using std::begin; using std::end;
                    auto stage_buf = arr::HostArray<T, vuh::mem::AllocDevice<vuh::mem::properties::HostCached>>(Base::_dev
                            , offset_end - offset_begin);
                    copyImageToBuffer(Base::_dev, *this, stage_buf, Base::width(), Base::height(), offset_begin);
                    std::copy(begin(stage_buf), end(stage_buf), dst_begin);
                }
            }

            /// @return host container with a copy of array data.
            template<class C, typename=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>>
            auto toHost() const-> C {
                auto ret = C(Base::size());
                using std::begin;
                toHost(begin(ret));
                return ret;
            }

            /// doc me
            auto device_begin()-> ImageIter<Device2DImage> { return ImageIter<Device2DImage>(*this, 0); }
            auto device_begin() const-> ImageIter<Device2DImage> { return ImageIter<Device2DImage>(*this, 0); }

            /// doc me
            auto device_end()-> ImageIter<Device2DImage> {return ImageIter<Device2DImage>(*this, Base::size());}
            auto device_end() const-> ImageIter<Device2DImage> {return ImageIter<Device2DImage>(*this, Base::size());}
        private: // helpers
            auto host_data()-> T* {
                assert(Base::isHostVisible());
                auto data = Base::_dev.mapMemory(Base::_mem, 0, Base::size_bytes());
#ifdef VULKAN_HPP_NO_EXCEPTIONS
                VULKAN_HPP_ASSERT(vhn::Result::eSuccess == data.result);
                if (vhn::Result::eSuccess == data.result) {
                    return static_cast<T *>(data.value);
                }
                return nullptr;
#else
                return static_cast<T*>(data);
#endif
            }

            auto host_data() const-> const T* {
                assert(Base::isHostVisible());
                auto data = Base::_dev.mapMemory(Base::_mem, 0, Base::size_bytes());
#ifdef VULKAN_HPP_NO_EXCEPTIONS
                VULKAN_HPP_ASSERT(vhn::Result::eSuccess == data.result);
                if (vhn::Result::eSuccess == data.result) {
                    return static_cast<const T *>(data.value);
                }
                return nullptr;
#else
                return static_cast<const T*>(data);
#endif
            }
        }; // class Device2DImage

        /// doc me
        template<class T, class Alloc>
        auto device_begin(Device2DImage<T, Alloc>& image)-> ImageIter<Device2DImage<T, Alloc>> {
            return image.device_begin();
        }

        /// doc me
        template<class T, class Alloc>
        auto device_end(Device2DImage<T, Alloc>& image)-> ImageIter<Device2DImage<T, Alloc>> {
            return image.device_end();
        }
    } // namespace img
} // namespace vuh
