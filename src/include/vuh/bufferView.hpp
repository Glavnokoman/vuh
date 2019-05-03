#pragma once

#include <vulkan/vulkan.h>
#include <cassert>

namespace vuh {
	/// Read-write view into the continuous portion of some vuh::Buffer
	/// Maybe used in place of array references in copy routines and kernel invocations
	/// to pass parts of the buffer data.
	template<class Buf>
	class BufferView {
	public:
		using buffer_type = Buf;
		using value_type = typename Buf::value_type;
		static constexpr auto descriptor_class = Buf::descriptor_class;

		BufferView(Buf& buf): _buf(buf), _offset(0u), _size(buf.size()){}

		explicit BufferView(Buf& buffer, std::size_t offset, std::size_t num_elements)
		   : _buf(&buffer), _offset(offset), _size(num_elements)
		{}

		/// @return reference to Vulkan buffer of the corresponding array
		auto buffer()-> VkBuffer { return *_buf; }
		/// @return offset (number of elements) of the beggining of the span wrt to buffer
		auto offset() const-> std::size_t {return _offset;}
		/// @return number of elements in the view
		auto size() const-> std::size_t {return _size;}
		/// @return number of bytes in the view
		auto size_bytes() const-> std::size_t {return size()*sizeof(value_type);}
	private: // data
		Buf* _buf;            ///< referes to underlying array object
		std::size_t _offset;  ///< offset (number of array elements) of the beginning of the span
		std::size_t _size;    ///< number of elements
	}; // class ArrayView

	/// Create a ArrayView into given Array.
	template<class Buf>
	auto buffer_view(Buf& buf, std::size_t offset, size_t num_elements)-> BufferView<Buf>{
		return BufferView<Buf>(buf, offset, num_elements);
	}
} // namespace vuh
