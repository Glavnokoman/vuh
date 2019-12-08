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

        class BasicMemory : virtual public vuh::core {
        public:
            BasicMemory() {};
            virtual auto bufferDescriptor() -> vhn::DescriptorBufferInfo& { return _bufferDescriptor; };
            virtual auto imageDescriptor() -> vhn::DescriptorImageInfo& { return _imageDescriptor; };
            virtual auto descriptorType() const -> vhn::DescriptorType { return vhn::DescriptorType::eCombinedImageSampler; };

        private:
            vhn::DescriptorBufferInfo  _bufferDescriptor;
            vhn::DescriptorImageInfo   _imageDescriptor;
        };
    } // namespace mem
} // namespace vuh
