#pragma once

#include <vulkan/vulkan.hpp>
#include <vuh/device.h>

#include <cassert>

namespace vuh {
	///
	class Fence: public vk::Fence {
	public:
		///
		Fence(): _device(nullptr) {}

		///
		explicit Fence(vk::Fence fence, vuh::Device& device)
		   : vk::Fence(fence) , _device(&device)
		{}

		Fence(const Fence&) = delete;
		auto operator= (const Fence&)-> Fence& = delete;

		///
		Fence(Fence&& other) noexcept
		   : vk::Fence(other), _device(other._device)
		{
			other._device = nullptr;
		}

		///
		auto operator= (Fence&& other) noexcept-> Fence& {
			this->swap(other);
			return *this;
		}

		///
		~Fence() noexcept {
			if(_device){
				_device->waitForFences({*this}, true, uint64_t(-1)); // wait forever
				_device->destroyFence(*this);
			}
		}

		/// member-wise swap
		auto swap(Fence& other) noexcept-> void{
			using std::swap;
			swap(static_cast<vk::Fence&>(*this), static_cast<vk::Fence&>(other));
			swap(this->_device, other._device);
		}

		/// doc me
		auto wait(size_t period=size_t(-1))-> void {
			if(_device){
				_device->waitForFences({*this}, true, period);
			}
		}
	private: // data
		vuh::Device* _device;
	}; // class Fence
} // namespace vuh
