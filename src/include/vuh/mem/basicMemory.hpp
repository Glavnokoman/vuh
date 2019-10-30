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
    } // namespace mem
} // namespace vuh
