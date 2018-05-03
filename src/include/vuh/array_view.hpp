#pragma once

#include "traits.hpp"

#include <cstdint>
#include <stdexcept>

namespace vuh {
   ///
template<class T>
class array_view {
   using value_type = T;
   using pointer_type = const T*;
   using reference_type = const T&;
   using size_type = std::size_t;
private: // data
   const pointer_type _begin;
   const pointer_type _end;
public:

   constexpr array_view(const T* begin, const T* end) noexcept
      : _begin(begin), _end(end) {}
   constexpr array_view(const T* begin, size_type size) noexcept
      : _begin(begin), _end(begin + size){}
   
   template<std::size_t N>
   constexpr array_view(const value_type(&arr)[N]) noexcept: _begin(arr), _end(arr + N){}
   
   template<class C>
   constexpr array_view(const C& c
                        , typename std::enable_if_t<traits::is_iterable<C>::value>* = nullptr) 
                       noexcept
      : _begin(c.data()), _end(c.data() + c.size()) 
   {}
   
   constexpr auto begin() const noexcept-> pointer_type { return cbegin(); }
   constexpr auto end() const noexcept-> pointer_type { return cend(); }
   constexpr auto cbegin() const noexcept-> pointer_type { return _begin; }
   constexpr auto cend() const noexcept-> pointer_type { return _end; }
   
   constexpr auto size() const noexcept-> size_type { return _end - _begin; }
   constexpr auto empty() const noexcept-> bool { return _begin = _end; }
   constexpr auto data() const noexcept->pointer_type { return _begin; }
   
   constexpr auto operator[](size_type i) const noexcept-> reference_type { return *(_begin + i); }
   
   constexpr auto at(size_type i) const-> reference_type {
      if(size() <= i)
         throw std::out_of_range("array_view1d::at");
      return *(_begin + i);
   }
}; // class array_view
} // namespace vuh
