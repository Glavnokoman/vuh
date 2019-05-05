#pragma once

#include "device.hpp"

#include <vulkan/vulkan.h>

#include <cassert>

namespace vuh {
	/// Read-write view into the continuous portion of some vuh::Buffer
	/// Maybe used in place of buffers in copy routines and kernel invocations
	/// to pass a subset of the buffer data.
	template<class Buf>
	class BufferSpan {
	public:
		using buffer_type = Buf;
		using value_type = typename Buf::value_type;
		static constexpr auto descriptor_class = Buf::descriptor_class;

		BufferSpan(const Buf& buf): _buf(buf), _offset(0u), _size(buf.size()){}

		explicit BufferSpan(Buf& buffer, std::size_t offset, std::size_t num_elements)
		   : _buf(&buffer), _offset(offset), _size(num_elements)
		{}

		/// @return reference to Vulkan buffer of the corresponding array
		auto buffer() const-> VkBuffer { return *_buf; }

		/// @return offset (number of elements) of the beggining of the span wrt to buffer
		auto offset() const-> std::size_t {return _offset;}
		auto offset_bytes() const-> std::size_t { return _offset*sizeof(value_type);}

		/// @return number of elements in the view
		auto size() const-> std::size_t {return _size;}
		auto size_bytes() const-> std::size_t {return size()*sizeof(value_type);}

		auto device() const-> const vuh::Device& {return _buf->device();}

		auto host_data() const { return _buf->host_data();}
		auto host_visible() const-> bool { return _buf->host_visible();}
	private: // data
		Buf* _buf;            ///< referes to underlying array object
		std::size_t _offset;  ///< offset (number of array elements) of the beginning of the span
		std::size_t _size;    ///< number of elements
	}; // class ArrayView

	/// Create a ArrayView into given Array.
	template<class Buf>
	auto span(Buf& buf, std::size_t offset, std::size_t num_elements)-> BufferSpan<Buf>{
		return BufferSpan<Buf>(buf, offset, num_elements);
	}
} // namespace vuh
