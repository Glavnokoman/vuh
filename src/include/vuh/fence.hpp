#pragma once

#include <vulkan/vulkan.hpp>
#include <vuh/device.h>

#include <cassert>

namespace vuh {
	namespace detail{
		struct Noop{ auto operator()() const noexcept-> void{}; };
		
		template<class T>
		struct ExecutionCounter{
			auto setExecuted()-> void { _flag = true;}
			auto isExecuted() const-> bool {return _flag;}
		private:
			bool _flag = false;
		};
		
		template<>
		struct ExecutionCounter<Noop> {
			static constexpr auto setExecuted()-> void {}
			static constexpr auto isExecuted() -> bool {return true;}
		};
	}

	/// Action delayed by vulkan fence.
	/// If no action specified just wait till the unerlying fence is signalled.
	template<class Action=detail::Noop>
	class Delayed: public vk::Fence, private Action, private detail::ExecutionCounter<Action> {
		template<class> friend class Delayed;
	public:
		Delayed(): detail::ExecutionCounter<Action>(), _device{nullptr} {}
		Delayed(vk::Fence fence, vuh::Device& device, Action action={})
		   : vk::Fence(fence)
		   , Action(action)
		   , detail::ExecutionCounter<Action>()
		   , _device(&device)
		{}
		explicit Delayed(Delayed<detail::Noop>&& noop, Action action={})
		   : vk::Fence(std::move(noop)), Action(std::move(action)), _device(noop._device)
		{
			noop._device = nullptr;
		}

		~Delayed(){ 
			if(_device){
				_device->waitForFences({*this}, true, size_t(-1));
				_device->destroyFence(*this);
			}
			if(!detail::ExecutionCounter<Action>::isExecuted()){
				static_cast<Action&>(*this)(); /// exercise action
			}
		}

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
			}
			static_cast<Action&>(*this)(); /// exercise action
			detail::ExecutionCounter<Action>::setExecuted();
		}
	private: // data
		vuh::Device* _device;
	}; // class Delayed

	using Fence = Delayed<detail::Noop>;
} // namespace vuh
