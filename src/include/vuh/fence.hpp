#pragma once

#include <vulkan/vulkan.hpp>
#include <vuh/device.h>

#include <cassert>

namespace vuh {
	namespace detail{
		struct Noop{ auto operator()() const noexcept-> void{}; };
	}

	/// Action delayed by vulkan fence.
	/// If no action specified just wait till the unerlying fence is signalled.
	template<class Action=detail::Noop>
	class Delayed: public vk::Fence, private Action {
	public:
		explicit Delayed(): _device{nullptr}{}
		explicit Delayed(vk::Fence fence, vuh::Device& device, Action action={})
		   :vk::Fence(fence), Action(action), _device(&device)
		{}
		explicit Delayed(Delayed<>&& noop, Action action={})
		   : vk::Fence(noop), Action(action), _device(noop._device)
		{
			noop._device = nullptr;
		}

		~Delayed(){ wait(); }

		Delayed(const Delayed&) = delete;
		auto operator= (const Fence&)-> Fence& = delete;

		///
		Delayed(Delayed&& other) noexcept
		   : vk::Fence(std::move(other)), Action{std::move(other)}, _device(other._device)
		{
			other._device = nullptr;
		}

		auto operator= (Delayed&& other) noexcept-> Delayed& {
			this->swap(other);
			return *this;
		}
		auto swap(Delayed& other) noexcept-> void {
			using std::swap;
			swap(static_cast<vk::Fence&>(*this), static_cast<vk::Fence&>(other));
			swap(static_cast<Action&>(*this), static_cast<Action&>(other));
			swap(this->_device, other._device);
		}

		auto wait(size_t period=size_t(-1))-> void {
			if(_device){
				_device->waitForFences({*this}, true, period);
				_device->destroyFence(*this);
				_device = nullptr;
				static_cast<Action&>(*this)(); /// exercise action
			}
		}
	private: // data
		vuh::Device* _device;
	}; // class Delayed

	using Fence = Delayed<>;
} // namespace vuh
