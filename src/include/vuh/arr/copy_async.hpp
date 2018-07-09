#pragma once

#include "fence.hpp"

namespace vuh {

	/// copy data
	template<class Begin, class End, class ArrayIter>
	auto copy_async(Begin begin, End end, ArrayIter array_iter)-> vuh::Fence {
		throw "not implemented";
	}
} // namespace vuh
