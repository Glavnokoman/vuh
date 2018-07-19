#pragma once

#include "arrayIter.hpp"

#include <iterator>

namespace vuh {
namespace arr {
	///
	template<class Array>
	class HostArrayIterator: public ArrayIter<Array> {
		using array_type = Array;
		using value_type = typename Array::value_type;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;
		using iterator_category = std::random_access_iterator_tag;
	public:
		explicit HostArrayIterator(Array& array);
		HostArrayIterator(Array& array, std::size_t offset);

		auto swap(HostArrayIterator& other)-> void;
		auto operator*() const-> value_type;
		auto operator*()-> reference;
		auto operator++()-> HostArrayIterator&;
		auto operator++(int)-> HostArrayIterator;
		auto operator--()-> HostArrayIterator&;
		auto operator--(int)-> HostArrayIterator;
		auto operator+=(difference_type)-> HostArrayIterator&;
		auto operator-=(difference_type)-> HostArrayIterator&;
		auto operator-=(const HostArrayIterator&)-> difference_type;
		auto operator[](size_t) const-> value_type;
		auto operator[](size_t)-> value_type&;
		auto operator<(const HostArrayIterator&)-> bool;
	private: // data
		array_type* _array;
		size_t _offset;
	}; // class HostArrayIterator
} // namespace arr
} // namespace vuh
