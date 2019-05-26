#pragma once

#include <utility>

namespace vuh::utils {
///
template<class Deleter>
struct Deferred {
	explicit Deferred(Deleter d): _d(std::move(d)){}
	~Deferred(){ _d(); }

	Deferred(const Deferred&)=delete;
	auto operator= (const Deferred&)->Deferred& =delete;
private: // data
	Deleter _d;
};

template<class D> auto defer(D&& d){return Deferred(std::forward<D>(d));}
} // namespace vuh
