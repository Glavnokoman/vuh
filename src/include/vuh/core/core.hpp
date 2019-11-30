#pragma once

#include "vnh.hpp"

namespace vuh {

    class core {
    public:
        core() : _res(vhn::Result::eSuccess) { }
        explicit core(vhn::Result res) : _res(res) { }

    public:
        virtual vhn::Result error() const { return _res; };
        virtual bool success() const { return vhn::Result::eSuccess == _res; }
        virtual std::string error_to_string() const { return vhn::to_string(_res); };

    protected:
        vhn::Result                _res; ///< result of vulkan's api
    };
}

