#pragma once

#include <cstdint>
#include <vector>

namespace vuh {
	/// Typelist. That is all about it.
	template<class... Ts> struct typelist{};

	/// @return nearest integer bigger or equal to exact division value
	inline auto div_up(uint32_t x, uint32_t y){ return (x + y - 1u)/y; }

	auto read_spirv(const char* filename)-> std::vector<char>;

} // namespace vuh
