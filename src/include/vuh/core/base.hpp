#pragma once

#include "core.hpp"
#include <string>

namespace vuh {

    class base {
    public:
        base() : _res(vhn::Result::eSuccess) { }
        explicit base(vhn::Result res) : _res(res) { }

    public:
        virtual vhn::Result error() const { return _res; };
        virtual bool success() const { return vhn::Result::eSuccess == _res; }
        virtual std::string error_to_string() const { return vhn::to_string(_res); };

    protected:
        vhn::Result                _res; ///< result of vulkan's api
    };
}
