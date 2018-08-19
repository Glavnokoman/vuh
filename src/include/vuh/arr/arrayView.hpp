#pragma once

#include <cassert>

namespace vuh {
	/// doc me
	template<class Array>
	class ArrayView {
	public:
		using array_type = Array;
		using value_type = typename Array::value_type;
		static constexpr auto descriptor_class = Array::descriptor_class;

		/// doc me
		explicit ArrayView(Array& array, std::size_t offset_begin, std::size_t offset_end)
		   : _array(&array), _offset_begin(offset_begin), _offset_end(offset_end)
		{}

		auto buffer()-> vk::Buffer& { return *_array; }
		auto offset() const-> std::size_t {return _offset_begin;}
		auto size() const-> std::size_t {return _offset_end - _offset_begin;}
		auto size_bytes() const-> std::size_t {return size()*sizeof(value_type);}
	private: // data
		Array* _array;             ///< doc me
		std::size_t _offset_begin; ///< doc me
		std::size_t _offset_end;   ///< doc me
	}; // class ArrayView

	/// doc me
	template<class Array>
	auto array_view(Array& array, std::size_t offset_begin, size_t offset_end)-> ArrayView<Array>{
		return ArrayView<Array>(array, offset_begin, offset_end);
	}
} // namespace vuh
