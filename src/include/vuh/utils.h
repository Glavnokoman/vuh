#pragma once

#include <cstdint>
#include <vector>

namespace vuh {
	template<class... Ts> struct typelist{};

	/// doc me
	inline auto div_up(uint32_t x, uint32_t y){ return (x + y - 1u)/y; }

	auto read_spirv(const char* filename)-> std::vector<char>;

} // namespace vuh
