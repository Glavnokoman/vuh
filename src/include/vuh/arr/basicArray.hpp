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
			static constexpr auto descriptor_class = basic_memory_array_clz;

			/// Construct SBO array of given size in device memory
			BasicArray(vuh::Device& dev                     ///< device to allocate array
					   , size_t n_elements                     ///< number of elements
					   , vhn::MemoryPropertyFlags props={} ///< additional memory property flags. These are 'added' to flags defind by allocator.
					   , vhn::BufferUsageFlags usage={}         ///< additional usage flagsws. These are 'added' to flags defined by allocator.
					   )
			   : vhn::Buffer(Alloc::makeBuffer(dev, n_elements * sizeof(T), descriptor_flags | usage, _res))
			   , _dev(dev)
			   , _size(n_elements)
		   {
		#ifdef VULKAN_HPP_NO_EXCEPTIONS
				VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _res);
				if (vhn::Result::eSuccess == _res) {
					auto alloc = Alloc();
					_mem = alloc.allocMemory(dev, *this, props);
					_flags = alloc.memoryProperties(dev);
					_dev.bindBufferMemory(*this, _mem, 0);
				} else {
					release();
				}
		#else
			  try{
				 auto alloc = Alloc();
				 _mem = alloc.allocMemory(_dev, *this, props);
				 _flags = alloc.memoryProperties(_dev);
				 _dev.bindBufferMemory(*this, _mem, 0);
			  } catch(std::runtime_error&){ // destroy buffer if memory allocation was not successful
				 release();
				 throw;
			  }
		#endif
			}

			/// Release resources associated with current object.
			~BasicArray() noexcept {release();}

		   BasicArray(const BasicArray&) = delete;
		   BasicArray& operator= (const BasicArray&) = delete;

			/// Move constructor. Passes the underlying buffer ownership.
			BasicArray(BasicArray&& other) noexcept
			   : vuh::mem::BasicMemory()
			   , vhn::Buffer(other), _mem(other._mem), _flags(other._flags), _dev(other._dev)
			{
				static_cast<vhn::Buffer&>(other) = nullptr;
			}

			/// @return underlying buffer
			auto buffer()-> vhn::Buffer { return *this; }

			/// @return offset of the current buffer from the beginning of associated device memory.
			/// For arrays managing their own memory this is always 0.
			auto offset() const-> std::size_t { return 0;}

			/// @return number of elements
			auto size() const-> size_t {return _size;}

			/// @return size of array in bytes.
			auto size_bytes() const-> uint32_t { return _size*sizeof(T); }

			/// @return reference to device on which underlying buffer is allocated
			auto device()-> vuh::Device& { return _dev; }

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
				return descriptor_class;
			}

			/// Move assignment.
			/// Resources associated with current array are released immidiately (and not when moved from
			/// object goes out of scope).
			auto operator= (BasicArray&& other) noexcept-> BasicArray& {
				release();
				_mem = other._mem;
				_flags = other._flags;
				_dev = other._dev;
				reinterpret_cast<vhn::Buffer&>(*this) = reinterpret_cast<vhn::Buffer&>(other);
				reinterpret_cast<vhn::Buffer&>(other) = nullptr;
				return *this;
			}

			/// swap the guts of two basic arrays
			auto swap(BasicArray& other) noexcept-> void {
				using std::swap;
				swap(static_cast<vhn::Buffer&>(&this), static_cast<vhn::Buffer&>(other));
				swap(_mem, other._mem);
				swap(_flags, other._flags);
				swap(_dev, other._dev);
			}
		private: // helpers
			/// release resources associated with current BasicArray object
			auto release() noexcept-> void {
				if(static_cast<vhn::Buffer&>(*this)) {
					if (bool(_mem)) {
						_dev.freeMemory(_mem);
					}
					_dev.destroyBuffer(*this);
				}
			}
		protected: // data
			vuh::Device& 			   _dev;               ///< referes underlying logical device
			vhn::DeviceMemory          _mem;           ///< associated chunk of device memory
			vhn::MemoryPropertyFlags   _flags;  ///< actual flags of allocated memory (may differ from those requested)
			uint32_t  				   _size; ///< number of elements
		}; // class BasicArray
	} // namespace arr
} // namespace vuh
