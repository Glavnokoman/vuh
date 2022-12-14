#pragma once

#include <cassert>
#include <utility>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>

#define VUH_TRY(expression, identifier) \
auto maybe_##identifier = (expression); \
if((maybe_##identifier).is_error()) \
	return (maybe_##identifier).error(); \
auto identifier = std::move(maybe_##identifier).value()

#define VUH_TRY_VOID(expression) \
	{\
	auto result = (expression); \
	if(result.is_error())\
		return result.error();\
	}

#define VUH_TRY_ELSE(expression, identifier, return_value) \
    auto maybe_##identifier = (expression); \
    if((maybe_##identifier).is_error()) \
        return return_value; \
    auto identifier = std::move(maybe_##identifier).value();

#define VUH_TRY_TEST(expression, identifier, message) \
    auto maybe_##identifier = (expression); \
    REQUIRE(!(maybe_##identifier).is_error()); \
    auto identifier = std::move(maybe_##identifier).value();


namespace vuh {
    template<typename T>
    class [[nodiscard]] Result
    {
        using R = vk::Result;

        public:
            Result(vk::Result ec)
                :_error_code(ec), _no_value{'\0'}
            {
                //Cannot have success but no value. Use Result<void> for that
                assert(ec != R::eSuccess);
            }

            Result(T value)
                :_error_code(R::eSuccess), _maybe_value{std::move(value)}
            {}

            Result(vk::ResultValue<T> result)
                :_error_code(result.result)
            {
                if(error() == R::eSuccess) {
                    _maybe_value = std::move(result.value);
                } else {
                    _no_value = '\1';
                }
            }

            Result(Result&& other) noexcept
                :_error_code{other._error_code}
            {
                if(is_error()) {
                    _no_value = '\2';
                } else {
                    _maybe_value = std::move(other._maybe_value);
                }
            }

            Result& operator=(Result&& other) noexcept
            {
                _error_code = other._error_code;

                 if(is_error()) {
                    _no_value = '\3';
                } else {
                    _maybe_value = std::move(other._maybe_value);
                }

                return *this;
            }

            Result(const Result&)=delete;
            Result& operator=(const Result&)=delete;

            auto error() const -> vk::Result
            {
                return _error_code;
            }

            auto is_error() const -> bool
            {
                return error() != R::eSuccess;
            }

            auto value() const& -> T {
                //FIXME: assert that there is a value?
                return _maybe_value;
            }

            auto value() && -> T {
                //FIXME: assert that there is a value?
                return std::move(_maybe_value);
            }

            ~Result()
            {
                if(!is_error())
                    _maybe_value.~T();
            }

        private:
            R _error_code;

            union {
                T _maybe_value;
                char _no_value;
            };
   };

   template<typename T>
   class [[nodiscard]] Result<T&> : public Result<std::reference_wrapper<T>>
   {
        using Base = Result<std::reference_wrapper<T>>;
    public:
        using Base::Base;

        Result(T& ref)
            :Base(std::reference_wrapper<T>{ref})
        {}
   };

   template<>
   class [[nodiscard]] Result<void>
   {
        using R = vk::Result;

    public:
        Result(vk::Result ec)
            :_error_code(ec)
        {}

        template<typename U>
        Result(vk::ResultValue<U> result)
            :_error_code(result.result)
        {}

        auto is_error() const noexcept -> bool
        {
            return error() != R::eSuccess;
        }

        auto error() const noexcept -> vk::Result
        {
            return _error_code;
        }

    private:
        R _error_code;
   };
}
