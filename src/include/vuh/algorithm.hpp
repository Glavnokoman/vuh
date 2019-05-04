#pragma once

#include "bufferDevice.hpp"
#include "bufferHost.hpp"
#include "bufferSpan.hpp"

#include "traits.hpp"

namespace vuh {

///
template<class T, class Alloc, class OutputIt>
auto copy(BufferSpan<BufferDevice<T, Alloc>> view, OutputIt dst)-> OutputIt;

///
template<class T, class Alloc, class OutputIt>
auto copy(BufferSpan<BufferHost<T, Alloc>> data, OutputIt dst)-> OutputIt;

///
template<class T, class Alloc, class OutputIt>
auto copy(const BufferDevice<T, Alloc>& buf, OutputIt dst)-> OutputIt;

///
template<class T, class Alloc, class OutputIt>
auto copy(const BufferHost<T, Alloc>& data, OutputIt dst)-> OutputIt;


///
template<class T, class Alloc, class InputIt>
auto copy(InputIt first, InputIt last, BufferDevice<T, Alloc>& buf)-> InputIt;

///
template<class T, class Alloc, class InputIt>
auto copy(InputIt first, InputIt last, BufferSpan<BufferDevice<T, Alloc>> data)-> InputIt;

///
template<class T, class Alloc, class InputIt>
auto copy(InputIt first, InputIt last, BufferHost<T, Alloc>& buf)-> InputIt;

///
template<class T, class Alloc, class InputIt>
auto copy(InputIt first, InputIt last, BufferSpan<BufferHost<T, Alloc>> data)-> InputIt;


///
template<class T, class Alloc, class InputIt, class F>
auto transform(InputIt first, InputIt last, BufferDevice<T, Alloc>& buf, F&& f)-> InputIt;

///
template<class T, class Alloc, class InputIt, class F>
auto transform(InputIt first, InputIt last, BufferSpan<BufferDevice<T, Alloc>> data, F&& f)-> InputIt;

///
template<class T, class Alloc, class OutputIt, class F>
auto transform(const BufferDevice<T, Alloc>& buf, OutputIt first, F&& f)-> OutputIt;

///
template<class T, class Alloc, class OutputIt, class F>
auto transform(BufferSpan<BufferDevice<T, Alloc>> data, OutputIt first, F&& f)-> OutputIt;


///
template<class C, class T, class Alloc
        , typename=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>
        >
auto to_host(BufferSpan<BufferDevice<T, Alloc>> data)-> C;

///
template<class C, class T, class Alloc
        , typename=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>
        >
auto to_host(const BufferDevice<T, Alloc>& buf)-> C;

///
template<class C, class F, class T, class Alloc
        , typename=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>
        >
auto to_host(BufferSpan<BufferDevice<T, Alloc>> data, F&& f)-> C;

///
template<class C, class F, class T, class Alloc
        , typename=typename std::enable_if_t<vuh::traits::is_iterable<C>::value>
        >
auto to_host(const BufferDevice<T, Alloc>& buf, F&& f)-> C;



} // namespace vuh
