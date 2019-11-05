#pragma once

#include "vuh/core/core.hpp"
#include <vuh/device.h>
#include <cassert>
#include <cstdint>

namespace vuh {
	/// Iterator to use in device-side copying and kernel calls.
	/// Not suitable for host-side algorithms, dereferencing, etc...
	template<class Array>
	class ArrayIter {
	public:
		using array_type = Array;
		using value_type = typename Array::value_type;

		/// Constructor
		explicit ArrayIter(Array& arr, std::size_t offset)
		   : _arr(&arr), _offset(offset)
		{}

		/// Swap two iterators
		auto swap(ArrayIter& other)-> void {
			using std::swap;
			swap(_arr, other._arr);
			swap(_offset, other._offset);
		}

		/// @return reference to device where iterated array is allocated
		auto device()-> vuh::Device& { return _arr->device(); }

		/// @return reference to Vulkan buffer of iterated array
		auto buffer()-> vhn::Buffer& { return *_arr; }

		/// @return offset (number of elements) wrt to beginning of the iterated array
		auto offset() const-> size_t { return _offset; }

		/// Increment iterator
		auto operator += (std::size_t offset)-> ArrayIter& {
			_offset += offset;
			return *this;
		}

		/// @return reference to undelying array
		auto array() const-> const Array& { return *_arr; }
		auto array()-> Array& {return *_arr;}

		/// Decrement iterator
		auto operator -= (std::size_t offset)-> ArrayIter& {
			assert(offset <= _offset);
			_offset -= offset;
			return *this;
		}
		/// @return true if iterators point to element at the same offset.
		/// Only iterators to the same array are legal to compare for equality.
		auto operator == (const ArrayIter& other)-> bool {
			assert(_arr == other._arr); // iterators should belong to same array to be comparable
			return _offset == other._offset;
		}
		auto operator != (const ArrayIter& other)-> bool {return !(*this == other);}

		friend auto operator+(ArrayIter iter, std::size_t offset)-> ArrayIter {
			return iter += offset;
		}

		friend auto operator-(ArrayIter iter, std::size_t offset)-> ArrayIter {
			return iter -= offset;
		}
		
		/// @return distance between the two iterators
		/// It is assumed that it1 <= it2.
		friend auto operator-(const ArrayIter& it1, const ArrayIter& it2)-> size_t {
			assert(it2.offset() <= it1.offset());
			return it1.offset() - it2.offset();
		}
	private: // data
		Array* _arr;       ///< refers to the array being iterated
		std::size_t _offset; ///< offset (number of elements) wrt to beginning of the iterated array
	}; // class ArrayIter
} // namespace vuh
