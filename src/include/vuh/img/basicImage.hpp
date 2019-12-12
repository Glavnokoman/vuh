#pragma once

#include "vuh/core/core.hpp"
#include "vuh/mem/allocDevice.hpp"
#include "vuh/mem/basicMemory.hpp"
#include <vuh/device.h>
#include <cassert>

namespace vuh {
    namespace img {

/// Covers basic array functionality. Wraps the Image.
/// Keeps the data, handles initialization, copy/move, common interface,
/// binding memory to buffer objects, etc...
        template<class T, class Alloc>
        class BasicImage: virtual public vuh::mem::BasicMemory, public vhn::Image {
            using Basic = vuh::mem::BasicMemory;
        public:
            /// Construct Image of given size in device memory
            BasicImage(const vuh::Device& dev                     ///< device to allocate array
                    , const vhn::ImageType& imT
                    , const vhn::ImageViewType& imVT
                    , const vhn::DescriptorType& imDesc
                    , const size_t imW    ///< desired width
                    , const size_t imH    ///< desired height
                    , const vhn::Format& imFmt=vhn::Format::eR32G32B32A32Sfloat/// format
                    , const vhn::ImageUsageFlags& imF={}         ///< additional usage flagsws. These are 'added' to flags defined by allocator.
            )
                    : vhn::Image(Alloc::makeImage(dev, imT, imW, imH, imFmt, imF, _res))
                    , _dev(dev)
                    , _imLayout(vhn::ImageLayout::eGeneral)
                    , _imDesc(imDesc)
                    , _imFmt(imFmt) {
                VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _res);
                do {
                    if (vhn::Result::eSuccess != _res)
                        break;
                    auto alloc = Alloc();
                    _mem = alloc.allocMemory(_dev, vhn::Image(*this), vhn::MemoryPropertyFlagBits::eDeviceLocal, _res);
                    if (vhn::Result::eSuccess != _res)
                        break;
                #ifdef VULKAN_HPP_NO_EXCEPTIONS
                    _res = _dev.bindImageMemory(vhn::Image(*this), _mem, 0);
                    if (vhn::Result::eSuccess != _res)
                        break;
                #else
                    _dev.bindImageMemory(vhn::Image(*this), _mem, 0);
                #endif
                    _imView = alloc.makeImageView(_dev, vhn::Image(*this), imVT, imFmt, _res);
                    if (vhn::Result::eSuccess != _res)
                        break;
                    _sampler = alloc.makeSampler(_dev, vhn::Filter::eNearest, vhn::SamplerAddressMode::eClampToBorder, _res);
                } while(0);
                if (vhn::Result::eSuccess != _res) { // destroy buffer if memory allocation was not successful
                    release();
                #ifndef VULKAN_HPP_NO_EXCEPTIONS
                    throw;
                #endif
                }
            }

            /// Release resources associated with current object.
            ~BasicImage() noexcept {release();}

            BasicImage(const BasicImage&) = delete;
            BasicImage& operator= (const BasicImage&) = delete;

            /// Move constructor. Passes the underlying buffer ownership.
            BasicImage(BasicImage&& other) noexcept
                    : vhn::Image(other)
                    , _mem(other._mem)
                    , _dev(other._dev)
                    , _imView(other._imView)
                    , _sampler(other._sampler)
                    , _imLayout(other._imLayout)
                    , _imDesc(other._imDesc)
                    , _imFmt(other._imFmt) {
                static_cast<vhn::Image&>(other) = nullptr;
                other._imView = nullptr;
                other._mem = nullptr;
                other._sampler = nullptr;
                other._imFmt = vhn::Format::eUndefined;
            }

            /// @return underlying image
            auto image() const -> const vhn::Image& { return *this; }

            auto imageView() const -> const vhn::ImageView& { return _imView; }

            auto imageLayout() const -> const vhn::ImageLayout& { return _imLayout; }

            auto imageFormat() const -> const vhn::Format& { return _imFmt; }

            auto sampler() const -> const vhn::Sampler& { return _sampler; }

            /// @return reference to device on which underlying buffer is allocated
            auto device()-> vuh::Device& { return const_cast<vuh::Device&>(_dev); }

            /// Move assignment.
            /// Resources associated with current array are released immidiately (and not when moved from
            /// object goes out of scope).
            auto operator= (BasicImage&& other) noexcept-> BasicImage& {
                release();
                _mem = other._mem;
                _dev = other._dev;
                _imView = other._imView;
                _sampler = other._sampler;
                _imLayout = other._imLayout;
                _imDesc = other._imDesc;
                _imFmt = other._imFmt;
                static_cast<vhn::Image&>(*this) = static_cast<vhn::Image&>(other);
                static_cast<vhn::Image&>(other) = nullptr;
                other._imView = nullptr;
                other._mem = nullptr;
                other._sampler = nullptr;
                other._dev = nullptr;
                other._imFmt = vhn::Format::eUndefined;
                return *this;
            }

            /// swap the guts of two basic arrays
            auto swap(BasicImage& other) noexcept-> void {
                using std::swap;
                swap(static_cast<vhn::Image&>(&this), static_cast<vhn::Image&>(other));
                swap(_mem, other._mem);
                swap(_dev, other._dev);
                swap(_imView, other._imView);
                swap(_sampler, other._sampler);
                swap(_imLayout, other._imLayout);
                swap(_imDesc, other._imDesc);
                swap(_imFmt, other._imFmt);
            }

            virtual auto imageDescriptor() -> vhn::DescriptorImageInfo& override {
                Basic::imageDescriptor().setSampler(sampler());
                Basic::imageDescriptor().setImageView(imageView());
                Basic::imageDescriptor().setImageLayout(imageLayout());
                return Basic::imageDescriptor();
            }

            virtual auto descriptorType() const -> vhn::DescriptorType override  {
                return _imDesc;
            }

        private: // helpers
            /// release resources associated with current BasicArray object
            auto release() noexcept-> void {
                if(bool(_sampler)) {
                    _dev.destroySampler(_sampler);
                    _sampler = nullptr;
                }
                if(bool(_imView)) {
                    _dev.destroyImageView(_imView);
                    _imView = nullptr;
                }
                vhn::Image& im = static_cast<vhn::Image&>(*this);
                if(bool(im)) {
                    _dev.destroyImage(im);
                    im = nullptr;
                }
                if(bool(_mem)) {
                    _dev.freeMemory(_mem);
                    _mem = nullptr;
                }
            }
        protected: // data
            vhn::DeviceMemory          _mem;      ///< associated chunk of device memory
            const vuh::Device&         _dev;      ///< referes underlying logical device
            vhn::ImageView             _imView;
            vhn::Sampler               _sampler;
            vhn::ImageLayout           _imLayout;
            vhn::DescriptorType        _imDesc;
            vhn::Format                _imFmt;
        }; // class BasicImage

        template<class T, class Alloc>
        class BasicImage2D: public BasicImage<T, Alloc> {
            using Base = BasicImage<T, Alloc>;
        public:
            /// Construct Image of given size in device memory
            BasicImage2D(const vuh::Device& dev                     ///< device to allocate array
                    , const size_t imW                     ///< desired width in bytes
                    , const size_t imH                     ///< desired height in bytes
                    , const vhn::Format& imFmt = vhn::Format::eR32G32B32A32Sfloat /// format
                    , const vhn::DescriptorType& imDesc = vhn::DescriptorType::eStorageImage
                    , const vhn::ImageUsageFlags& imF = {}         ///< additional usage flagsws. These are 'added' to flags defined by allocator.
            )
                    : Base(dev, vhn::ImageType::e2D, vhn::ImageViewType::e2D, imDesc, imW, imH, imFmt, imF)
                    , _imW(0 == imW ? 1 : imW)
                    , _imH(0 == imH ? 1 : imH)
            {}

            /// @return size of a memory chunk occupied by array elements
            /// (not the size of actually allocated chunk, which may be a bit bigger).
            auto size_bytes() const-> uint32_t {return _imH * _imW * sizeof(T);}

            /// @return number of elements
            auto size() const-> size_t {return _imW * _imH;}

            auto width() const-> uint32_t {return _imW;}

            auto height() const-> uint32_t {return _imH;}

        private: // helpers
            size_t                                      _imW;    ///< desired width in bytes
            size_t                                      _imH;   ///< desired height in bytes
        }; //// class BasicImage2D

    } // namespace img
} // namespace vuh
