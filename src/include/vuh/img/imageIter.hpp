#pragma once

#include <vuh/device.h>
#include <vulkan/vulkan.hpp>
#include <cassert>
#include <cstdint>

namespace vuh {
    /// Iterator to use in device-side copying and kernel calls.
    /// Not suitable for host-side algorithms, dereferencing, etc...
    template<class Image>
    class ImageIter {
    public:
        using array_type = Image;
        using value_type = typename Image::value_type;

        /// Constructor
        explicit ImageIter(Image& image, std::size_t offset)
                : _image(&image), _offset(offset)
        {}

        /// Swap two iterators
        auto swap(ImageIter& other)-> void {
            using std::swap;
            swap(_image, other._image);
            swap(_offset, other._offset);
        }

        /// @return reference to device where iterated image is allocated
        auto device()-> vuh::Device& { return _image->device(); }

        /// @return reference to Vulkan image of iterated array
        auto image()-> VULKAN_HPP_NAMESPACE::Buffer& { return *_image; }

        /// @return offset (number of elements) wrt to beginning of the iterated array
        auto offset() const-> size_t { return _offset; }

        /// Increment iterator
        auto operator += (std::size_t offset)-> ImageIter& {
            _offset += offset;
            return *this;
        }

        /// @return reference to undelying image
        auto image() const-> const Image& { return *_image; }
        //auto image()-> Image& {return *_image;}

        /// Decrement iterator
        auto operator -= (std::size_t offset)-> ImageIter& {
            assert(offset <= _offset);
            _offset -= offset;
            return *this;
        }
        /// @return true if iterators point to element at the same offset.
        /// Only iterators to the same array are legal to compare for equality.
        auto operator == (const ImageIter& other)-> bool {
            assert(_image == other._image); // iterators should belong to same array to be comparable
            return _offset == other._offset;
        }
        auto operator != (const ImageIter& other)-> bool {return !(*this == other);}

        friend auto operator+(ImageIter iter, std::size_t offset)-> ImageIter {
            return iter += offset;
        }

        friend auto operator-(ImageIter iter, std::size_t offset)-> ImageIter {
            return iter -= offset;
        }

        /// @return distance between the two iterators
        /// It is assumed that it1 <= it2.
        friend auto operator-(const ImageIter& it1, const ImageIter& it2)-> size_t {
            assert(it2.offset() <= it1.offset());
            return it1.offset() - it2.offset();
        }
    private: // data
        Image* _image;       ///< refers to the array being iterated
        std::size_t _offset; ///< offset (number of elements) wrt to beginning of the iterated image
    }; // class ImageIter
} // namespace vuh
