#pragma once

#include <cassert>

namespace vuh {
	/// Read-write view into the continuous portion of some vuh::Array
	/// Maybe used in place of array references in copy routines and kernel invocations
	/// to pass parts of the array data.
	template<class Array>
	class ArrayView {
	public:
		using array_type = Array;
		using value_type = typename Array::value_type;
		static constexpr auto descriptor_class = Array::descriptor_class;

		/// Constructor
		explicit ArrayView(Array& array, std::size_t offset_begin, std::size_t offset_end)
		   : _array(&array), _offset_begin(offset_begin), _offset_end(offset_end)
		{}

		/// @return reference to Vulkan buffer of the corresponding array
		auto buffer()-> vk::Buffer& { return *_array; }
		/// @return offset (number of elements) of the beggining of the span wrt to buffer
		auto offset() const-> std::size_t {return _offset_begin;}
		/// @return number of elements in the view
		auto size() const-> std::size_t {return _offset_end - _offset_begin;}
		/// @return number of bytes in the view
		auto size_bytes() const-> std::size_t {return size()*sizeof(value_type);}
	private: // data
		Array* _array;             ///< referes to underlying array object
		std::size_t _offset_begin; ///< offset (number of array elements) of the beginning of the span
		std::size_t _offset_end;   ///< offset (number of array elements) of the end (one past the last valid elements) of the span.
	}; // class ArrayView

	/// Create a ArrayView into given Array.
	template<class Array>
	auto array_view(Array& array, std::size_t offset_begin, size_t offset_end)-> ArrayView<Array>{
		return ArrayView<Array>(array, offset_begin, offset_end);
	}
} // namespace vuh
