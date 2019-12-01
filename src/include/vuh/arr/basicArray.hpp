#pragma once

#include "vuh/core/core.hpp"
#include "vuh/mem/allocDevice.hpp"
#include "vuh/mem/basicMemory.hpp"
#include <vuh/device.h>
#include <cassert>

namespace vuh {
	namespace arr {

		/// Covers basic array functionality. Wraps the SBO buffer.
		/// Keeps the data, handles initialization, copy/move, common interface,
		/// binding memory to buffer objects, etc...
		template<class T, class Alloc>
		class BasicArray: virtual public vuh::mem::BasicMemory, public vhn::Buffer {
			static constexpr auto descriptor_flags = vhn::BufferUsageFlagBits::eStorageBuffer;
		public:
			/// Construct SBO array of given size in device memory
			BasicArray(const vuh::Device& dev                     ///< device to allocate array
					   , const size_t n_elements                     ///< number of elements
					   , const vhn::MemoryPropertyFlags props={} ///< additional memory property flags. These are 'added' to flags defind by allocator.
					   , const vhn::BufferUsageFlags usage={}         ///< additional usage flagsws. These are 'added' to flags defined by allocator.
					   )
			   : vhn::Buffer(Alloc::makeBuffer(dev, n_elements * sizeof(T), descriptor_flags | usage, _res))
			   , _dev(dev)
			   , _size(n_elements) {
				VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _res);
				do {
					if (vhn::Result::eSuccess != _res)
						break;
					auto alloc = Alloc();
					_mem = alloc.allocMemory(dev, *this, props, _res);
					if (vhn::Result::eSuccess != _res)
						break;
					_flags = alloc.memoryProperties(dev, *this, props);
					_dev.bindBufferMemory(*this, _mem, 0);
				} while (0);
				if (vhn::Result::eSuccess != _res) { // destroy buffer if memory allocation was not successful
					release();
				#ifndef VULKAN_HPP_NO_EXCEPTIONS
					throw;
				#endif
				}
			}

			/// Release resources associated with current object.
			~BasicArray() noexcept {release();}

		   BasicArray(const BasicArray&) = delete;
		   BasicArray& operator= (const BasicArray&) = delete;

			/// Move constructor. Passes the underlying buffer ownership.
			BasicArray(BasicArray&& other) noexcept
			   : vhn::Buffer(other)
			   , _dev(other._dev)
			   , _mem(other._mem)
			   , _flags(other._flags)
			   , _size(other._size)
			{
				static_cast<vhn::Buffer&>(other) = nullptr;
                other._mem = nullptr;
			}

			/// @return underlying buffer
			auto buffer() const -> const vhn::Buffer& { return *this; }

			/// @return offset of the current buffer from the beginning of associated device memory.
			/// For arrays managing their own memory this is always 0.
			auto offset() const -> std::size_t { return 0;}

			/// @return number of elements
			auto size() const -> size_t {return _size;}

			/// @return size of array in bytes.
			auto size_bytes() const -> uint32_t { return _size*sizeof(T); }

			/// @return reference to device on which underlying buffer is allocated
            auto device()-> vuh::Device& { return const_cast<vuh::Device&>(_dev); }

			/// @return true if array is host-visible, ie can expose its data via a normal host pointer.
			auto isHostVisible() const-> bool {
				return bool(_flags & vhn::MemoryPropertyFlagBits::eHostVisible);
			}

			virtual auto descriptorBufferInfo() -> vhn::DescriptorBufferInfo& override {
				_descBufferInfo.setBuffer(buffer());
				_descBufferInfo.setOffset(offset());
				_descBufferInfo.setRange(size_bytes());
				return _descBufferInfo;
			}

			virtual auto descriptorType() const -> vhn::DescriptorType override  {
				return vhn::DescriptorType::eStorageBuffer;
			}

			/// Move assignment.
			/// Resources associated with current array are released immidiately (and not when moved from
			/// object goes out of scope).
			auto operator= (BasicArray&& other) noexcept-> BasicArray& {
				release();
                _dev = other._dev;
				_mem = other._mem;
				_flags = other._flags;
                _size = other._size;
				static_cast<vhn::Buffer&>(*this) = static_cast<vhn::Buffer&>(other);
                static_cast<vhn::Buffer&>(other) = nullptr;
				return *this;
			}

			/// swap the guts of two basic arrays
			auto swap(BasicArray& other) noexcept-> void {
				using std::swap;
				swap(static_cast<vhn::Buffer&>(&this), static_cast<vhn::Buffer&>(other));
                swap(const_cast<vuh::Device&>(_dev), const_cast<vuh::Device&>(other._dev));
				swap(_mem, other._mem);
				swap(_flags, other._flags);
                swap(_size, other._size);
			}
		private: // helpers
			/// release resources associated with current BasicArray object
			auto release() noexcept-> void {
				if(static_cast<vhn::Buffer&>(*this)) {
					if (bool(_mem)) {
						_dev.freeMemory(_mem);
					}
                    _mem = nullptr;
					_dev.destroyBuffer(*this);
                    static_cast<vhn::Buffer&>(*this) = nullptr;
				}
			}
		protected: // data
			const vuh::Device& 		   _dev;               ///< referes underlying logical device
			vhn::DeviceMemory          _mem;           ///< associated chunk of device memory
			vhn::MemoryPropertyFlags   _flags;  ///< actual flags of allocated memory (may differ from those requested)
			uint32_t  				   _size; ///< number of elements
		}; // class BasicArray
	} // namespace arr
} // namespace vuh
