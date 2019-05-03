#pragma once

#include "bufferDevice.hpp"
#include "bufferHost.hpp"
#include "bufferView.hpp"

namespace vuh {
template<class T, class Alloc, class IterDst>
auto copy(BufferView<BufferDevice<T, Alloc>> view, IterDst dst)-> IterDst;

template<class T, class Alloc, class IterDst>
auto copy(BufferView<BufferHost<T, Alloc>> data, IterDst dst)-> IterDst;



} // namespace vuh
