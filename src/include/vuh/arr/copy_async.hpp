#pragma once

//#include <vuh/program.hpp>
#include "arrayIter.hpp"

namespace vuh {
	/// async between the host-visible and device-local arrays
	template<class Array1, class Array2>
	auto copy_async(ArrayIter<Array1> src_begin, ArrayIter<Array1> src_end
	                , ArrayIter<Array2> dst_begin
	                )-> vuh::Fence
	{
		auto& src_device = src_begin.array().device();
		auto& dst_device = dst_begin.array().device();
		assert(src_device == dst_device);

		auto cmd_buf = src_device.transferCmdBuffer();
		cmd_buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		auto region = vk::BufferCopy(src_begin.offset(), dst_begin.offset(), src_end - src_begin); // src_offset, dst_offset, size
		cmd_buf.copyBuffer(src_begin.array(), dst_begin.offset(), 1, &region);
		cmd_buf.end();
		auto queue = device.transferQueue();
		auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buf);
		queue.submit({submit_info}, nullptr);
//		queue.waitIdle();

		throw "not implemented";
	}

	/// async copy data from host to device
	template<class SrcIter1, class SrcIter2, class Array>
	auto copy_async(SrcIter1 src_begin, SrcIter2 src_end, vuh::ArrayIter<Array> dst_begin)-> vuh::Fence {
		auto cmd_buf = dst_begin.device().transferCmdBuffer();
		const auto& array = dst_begin.array();
		if(array.isHostVisible()){ // normal copy, the function blocks till the copying is complete
			throw "not implemented";
		} else { // copy first to staging buffer and then async copy from staging buffer to device
			throw "not implemented";
		}
	}

	/// async copy data from device to host
	template<class Array, class DstIter>
	auto copy_async(ArrayIter<Array> src_begin, ArrayIter<Array> src_end
	                , DstIter dst_begin)-> vuh::Fence
	{
		throw "not implemented";
	}

} // namespace vuh
