#pragma once

#include "arrayIter.hpp"

#include <cassert>
#include <iterator>

namespace vuh {
namespace arr {
	///
	template<class Array>
	class HostArrayIterator {
	public:
		using array_type = Array;
		using value_type = typename Array::value_type;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::random_access_iterator_tag;

		explicit HostArrayIterator(Array& array)
		   : _ptr(array.data()), _array(&array)
		{}
		HostArrayIterator(Array& array, std::size_t offset)
		   : _ptr(array.data() + offset), _array(&array)
		{
			assert(offset <= array.size());
		}

		auto swap(HostArrayIterator& other)-> void {
			using std::swap;
			swap(_array, other._array);
			swap(_ptr, other._ptr);
		}

		auto operator->() const-> const pointer {
			return _ptr;
		}
		auto operator->()-> pointer {
			return _ptr;
		}
		auto operator*() const-> value_type { return *_ptr; }
		auto operator*()-> reference { return *_ptr; }
		auto operator++()-> HostArrayIterator& {
			++_ptr;
			return *this;
		}
		auto operator++(int)-> HostArrayIterator {
			auto r = *this;
			++_ptr;
			return r;
		};
		auto operator--()-> HostArrayIterator& {
			--_ptr;
			return *this;
		}
		auto operator--(int)-> HostArrayIterator {
			auto r = *this;
			--_ptr;
			return r;
		}
		auto operator+=(difference_type off)-> HostArrayIterator& {
			_ptr += off;
			return *this;
		}
		auto operator-=(difference_type off)-> HostArrayIterator& {
			_ptr -= off;
			return *this;
		}
		auto operator-=(const HostArrayIterator& other)-> difference_type {
			assert(_array == other._array);
			return _ptr - other._ptr;
		}
		auto operator[](size_t off) const-> value_type {
			return *(_ptr + off);
		}
		auto operator[](size_t off)-> value_type& {
			return *(_ptr + off);
		}
		auto operator==(const HostArrayIterator& other)-> bool {
			assert(_array == other._array);
			return _ptr == other._ptr;
		}
		auto operator!=(const HostArrayIterator& other)-> bool {
			return !(*this == other);
		}
		auto operator<(const HostArrayIterator& other)-> bool {
			assert(_array == other._array);
			return _ptr < other._ptr;
		}

		////
		friend auto operator+(HostArrayIterator it, difference_type off)-> HostArrayIterator {
			return it += off;
		}
		friend auto operator-(HostArrayIterator it, difference_type off)-> HostArrayIterator {
			return it -= off;
		}
		friend auto operator-(HostArrayIterator it1, HostArrayIterator& it2)-> difference_type {
			return it1 -= it2;
		}
		////////////////////////////////
		///
		auto device()-> vuh::Device& {
			assert(_array != nullpt);
			return _array->device();
		}
		///
		auto buffer()-> vk::Buffer& { return _array->buffer();}
		///
		auto offset() const-> size_t {
			assert(_array->data() <= _ptr);
			return std::size_t(_ptr - _array->data());
		}

	private: // data
		pointer _ptr;
		array_type* _array;
	}; // class HostArrayIterator
} // namespace arr
} // namespace vuh
