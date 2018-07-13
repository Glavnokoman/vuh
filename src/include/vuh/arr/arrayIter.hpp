#pragma once

#include <cassert>
#include <cstdint>

namespace vuh {
	/// doc me
	/// not really an iterator. no dereferencing, just an offset
	template<class Array>
	class ArrayIter {
		using value_type = typename Array::value_type;
	public:
		explicit ArrayIter(Array& array, std::size_t offset)
		   : _array(array), _offset(offset)
		{}

		auto operator += (std::size_t offset)-> ArrayIter& {
			_offset += offset;
			return *this;
		}

		auto operator -= (std::size_t offset)-> ArrayIter& {
			assert(offset <= _offset);
			_offset -= offset;
			return *this;
		}
		auto operator == (const ArrayIter& other)-> bool {
			assert(&_array == &other._array); // iterators should belong to same array to be comparable
			_offset == other._offset;
		}
		auto operator != (const ArrayIter& other)-> bool {return !(*this == other);}

		friend auto operator+(ArrayIter iter, std::size_t offset)-> ArrayIter {
			return iter += offset;
		}

		friend auto operator-(ArrayIter iter, std::size_t offset)-> ArrayIter {
			return iter -= offset;
		}
	private: // data
		Array& _array;
		std::size_t _offset;
	}; // class DeviceArrayIter
} // namespace vuh
