#pragma once

#include <stdint.h>
#include <vector>

#define VUH_TRY(expression, identifier) \
auto maybe_##identifier = (expression); \
if((maybe_##identifier).result != vk::Result::eSuccess) \
	return (maybe_##identifier).result; \
auto identifier = std::move((maybe_##identifier).value)


namespace vuh {
	/// Typelist. That is all about it.
	template<class... Ts> struct typelist{};

	/// @return nearest integer bigger or equal to exact division value
	inline auto div_up(uint32_t x, uint32_t y){ return (x + y - 1u)/y; }

	auto read_spirv(const char* filename)-> std::vector<uint32_t>;

} // namespace vuh
