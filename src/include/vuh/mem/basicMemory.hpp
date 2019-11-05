#pragma once

#include "allocDevice.hpp"
#include "memProperties.h"

namespace vuh {
    /// defines some shortcut allocators types
    namespace mem {
        using DeviceOnly = vuh::mem::AllocDevice<vuh::mem::properties::DeviceOnly>;
        using Device = vuh::mem::AllocDevice<vuh::mem::properties::Device>;
        using Unified = vuh::mem::AllocDevice<vuh::mem::properties::Unified>;
        using Host = vuh::mem::AllocDevice<vuh::mem::properties::Host>;
        using HostCached = vuh::mem::AllocDevice<vuh::mem::properties::HostCached>;
        using HostCoherent = vuh::mem::AllocDevice<vuh::mem::properties::HostCoherent>;

        class BasicMemory {
        public:
            static constexpr auto basic_memory_image_clz = vhn::DescriptorType::eStorageImage;
            static constexpr auto basic_memory_array_clz = vhn::DescriptorType::eStorageBuffer;
        public:
            virtual auto descriptorBufferInfo() -> vhn::DescriptorBufferInfo& { return descBufferInfo; };
            virtual auto descriptorImageInfo() -> vhn::DescriptorImageInfo& { return descImageInfo; };
            virtual auto descriptorType() const -> vhn::DescriptorType { return vhn::DescriptorType::eSampler; };

        protected:
            vhn::DescriptorBufferInfo  descBufferInfo;
            vhn::DescriptorImageInfo   descImageInfo;
        };
    } // namespace mem
} // namespace vuh
