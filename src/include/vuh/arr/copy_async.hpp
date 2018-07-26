#pragma once

//#include <vuh/program.hpp>
#include "arrayIter.hpp"

namespace vuh {

	/// async copy data from host to device
	template<class SrcIter1, class SrcIter2, class Array>
	auto copy_async(SrcIter1 src_begin, SrcIter2 src_end, vuh::ArrayIter<Array> dst_begin)-> vuh::Fence {
		auto cmd_buf = dst_begin.device().transferCmdBuffer();
		throw "not implemented";
	}

	/// async copy data from device to host
	template<class Array, class DstIter>
	auto copy_async(ArrayIter<Array> src_begin, ArrayIter<Array> src_end, DstIter dst_begin)-> vuh::Fence {
		throw "not implemented";
	}
} // namespace vuh
