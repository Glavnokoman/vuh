#pragma once

#include <vulkan/vulkan.hpp>
#include <vuh/device.h>
#include <vuh/movable.hpp>

#include <cassert>

namespace vuh {
	namespace detail{
		struct Noop{ constexpr auto operator()() const noexcept-> void{}; };
	}

	/// Action delayed by vulkan fence.
	/// If no action specified just wait till the unerlying fence is signalled.
	template<class Action=detail::Noop>
	class Delayed: public vk::Fence, private Action {
		template<class> friend class Delayed;
	public:
		Delayed(vk::Fence fence, vuh::Device& device, Action action={})
		   : vk::Fence(fence)
		   , Action(std::move(action))
		   , _device(&device)
		{}

		/// Constructor. Creates the fence in a signalled state.
		explicit Delayed(vuh::Device& device, Action action={})
		   : vk::Fence(device.createFence({vk::FenceCreateFlagBits::eSignaled}))
		   , Action(std::move(action))
		   , _device(&device)
		{}

		explicit Delayed(Delayed<detail::Noop>&& noop, Action action={})
		   : vk::Fence(std::move(noop)), Action(std::move(action)), _device(std::move(noop._device))
		{}

		~Delayed(){ release(); }

		Delayed(const Delayed&) = delete;
		auto operator= (const Delayed&)-> Delayed& = delete;
		Delayed(Delayed&& other) = default;

		auto operator= (Delayed&& other) noexcept-> Delayed& {
			release();
			static_cast<vk::Fence&>(*this) = std::move(static_cast<vk::Fence&>(other));
			static_cast<Action&>(*this) = std::move(static_cast<Action&>(other));
			_device = std::move(other._device);
			return *this;
		}

		/// doc me
		auto release() noexcept-> void {
			if(_device){
				_device->waitForFences({*this}, true, size_t(-1));
				_device->destroyFence(*this);
				static_cast<Action&>(*this)(); // exercise action
				_device.release();
			}
		}

		/// doc me
		auto wait(size_t period=size_t(-1))-> void {
			release();
		}
	private: // data
		std::unique_ptr<Device, util::NoopDeleter<Device>> _device;
	}; // class Delayed

	///
	using Fence = Delayed<detail::Noop>;
} // namespace vuh
