#pragma once

#include <cassert>
#include <random>

namespace test {

/// doc me
template<class T>
auto random_vector(size_t size, T low, T high)-> std::vector<T> {
	static auto gen = std::mt19937(42);
	auto dis = std::uniform_real_distribution<T>(low, high);
	auto ret = std::vector<T>{}.reserve(size);
	for(size_t i = size; i != 0u; --i){
		ret.push_back(dis(gen));
	}
	return ret;
}

///
template <class T>
auto saxpy(std::vector<T> y, const std::vector<T>& x, T a)-> std::vector<T> {
	assert(y.size() == x.size());
	auto ret = std::move(y);
	for(size_t i = 0; i < ret.size(); ++i){
		ret[i] += a*x[i];
	}
	return ret;
}
} // namespace test
