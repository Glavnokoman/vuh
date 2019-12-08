#pragma once

#include "vuh/mem/memProperties.h"
#include "vuh/mem/allocDevice.hpp"
#include "img/imageIter.hpp"
#include "img/deviceImage.hpp"
#include "mem/basicMemory.hpp"

namespace vuh {
    namespace detail {

        /// Traits to associate array properties to underlying Image classes (DeviceImage,DeviceOnlyImage).
        /// Default mapping is Image2DClz<T> -> arr::HostArray.
        template<class Props>
        struct Image2DClz {
            template<class T, class Alloc> using type = vuh::img::DeviceImage2D<T, Alloc>;
        };

        /// Explicit trait specialization mapping Image2DClz<arr::properties::Device> -> arr::Device2DImage
        template<>
        struct Image2DClz<vuh::mem::properties::Device>{
            template<class T, class Alloc> using type = vuh::img::DeviceImage2D<T, Alloc>;
        };

        template<class Props>
        struct CombinedImage2DClz {
            template<class T, class Alloc> using type = vuh::img::DeviceCombinedImage2D<T, Alloc>;
        };

        /// Explicit trait specialization mapping CombinedImage2DClz<arr::properties::Device> -> arr::Device2DImage
        template<>
        struct CombinedImage2DClz<vuh::mem::properties::Device>{
            template<class T, class Alloc> using type = vuh::img::DeviceCombinedImage2D<T, Alloc>;
        };
    } // namespace detail

    /// Maps Image classes with different data exchange interfaces, to a single template type.
    /// This enables type declarations of Images with different allocators.
    /// Although in this case resulting classes have different data exchange interfaces,
    /// this may still be useful since they all have similar binding properties to shaders.
    template<class T, class Alloc=vuh::mem::AllocDevice<vuh::mem::properties::Device>>
    using Image2D = typename detail::Image2DClz<typename Alloc::properties_t>::template type<T, Alloc>;

    template<class T, class Alloc=vuh::mem::AllocDevice<vuh::mem::properties::Device>>
    using CombinedImage2D = typename detail::CombinedImage2DClz<typename Alloc::properties_t>::template type<T, Alloc>;
} // namespace vuh
