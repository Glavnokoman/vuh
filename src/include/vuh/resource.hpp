#pragma once

#include <utility>

namespace vuh {
namespace util {
	/// Helper class to wrap some class with resource data and a release() function to clean after
	/// itself into a nice movable entity.
	template<class T>
	struct Resource: public T {
		template<class... Ts>
		Resource(Ts&&... args): T(std::forward<Ts>(args)...) {}

		~Resource() noexcept { T::release(); }

		Resource(const Resource&) = delete;
		auto operator= (const Resource&)-> Resource& = delete;
		Resource(Resource&&) = default;
		auto operator= (Resource&& o) noexcept-> Resource& {
			T::release();
			static_cast<T&>(*this) = std::move(static_cast<T&>(o));
			return *this;
		}
	}; // struct Resource

	/// Noop deleter.
	template<class T>
	struct NoopDeleter {
		constexpr auto operator()(T*) noexcept-> void {}
	}; // struct NoopDeleter

} // namespace util
} // namespace vuh
