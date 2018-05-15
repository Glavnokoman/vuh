#include "vuh/utils.h"

#include <fstream>

namespace vuh {
	/// Read binary shader file into array of uint32_t. little endian assumed.
	/// Padded by 0s to a boundary of 4.
	auto read_spirv(const char* filename)-> std::vector<char> {
		auto fin = std::ifstream(filename, std::ios::binary);
		if(!fin.is_open()){
			throw std::runtime_error(std::string("could not open file ") + filename);
		}
		auto ret = std::vector<char>(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());

		ret.resize(4u*div_up(uint32_t(ret.size()), 4u));
		return ret;
	}

} // namespace vuh
