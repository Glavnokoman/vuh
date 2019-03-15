#pragma once

#include <vulkan/vulkan.hpp>
#include <vuh/device.h>
#include <vuh/resource.hpp>
#include <vuh/fence.hpp>
#include <vuh/event.hpp>

#include <cassert>

namespace vuh {
	namespace detail{
		/// No action. Runnable with operator()() doing nothing.
		struct Noop{ constexpr auto operator()() const noexcept-> void{}; };
	}

	/// Class used for synchronization with host.
	/// Represent an action (maybe noop, which is actually the default) to trigger after
	/// the underlying vulkan fence is signalled,
	/// but not before the object of this class is put to a waiting state.
	/// May also carry resources which lifetime needs to be extended beyond the place of
	/// async function call creating an object of this class.
	/// The object is put the waiting state when its member wait() function is destructor
	/// is called (incl. implicitely, ie the object goes out of scope).
	/// Both wait() function and destructor calls are blocking till the underlying fence
	/// is signalled.
	/// If no fence is passed to the contructor of Delayed object the fence in signalled
	/// state is created under the hood.
	/// The corresponding action will necessarily take place once and only once, whether
	/// it is at the explicit wait() call or at object destruction.
	template<class Action=detail::Noop>
	class Delayed: public vuh::Fence, vuh::Event, private Action {
		template<class> friend class Delayed;
	public:
		/// Constructor. Takes ownership of the fence.
		/// It is assumed that the fence belongs to the same device that is passed together with it.
		Delayed(vuh::Fence& fence, vuh::Device& device, Action action={})
		   : vuh::Fence(fence)
		   , Action(std::move(action))
		   , _device(&device)
		   , _result(VULKAN_HPP_NAMESPACE::Result::eSuccess)
		{}

		explicit Delayed(vuh::Fence& fence, vuh::Device& device, VULKAN_HPP_NAMESPACE::Result result, Action action={})
				: vuh::Fence(fence)
				, Action(std::move(action))
				, _device(&device)
				, _result(result)
		{}

        explicit Delayed(vuh::Fence& fence, vuh::Event& event, vuh::Device& device, Action action={})
                : vuh::Fence(fence)
				, vuh::Event(event)
                , Action(std::move(action))
                , _device(&device)
                , _result(VULKAN_HPP_NAMESPACE::Result::eSuccess)
        {}

		/// Constructs for VULKAN_HPP_NO_EXCEPTIONS
		explicit Delayed(vuh::Device& device, VULKAN_HPP_NAMESPACE::Result result, vuh::Event& event, Action action={})
				: vuh::Fence()
				, vuh::Event(event)
				, Action(std::move(action))
				, _device(&device)
				, _result(result)
		{}

		/// Constructor. Creates the fence in a signalled state.
		explicit Delayed(vuh::Device& device, Action action={})
				: vuh::Fence(device, true)
				, Action(std::move(action))
				, _device(&device)
				, _result(VULKAN_HPP_NAMESPACE::Result::eSuccess)
		{}


		/// Constructs from the object of Delayed<Noop> and inherits the state of that.
		/// Takes over the undelying fence ownership.
		/// Mostly substitute its own action in place of Noop.
		explicit Delayed(Delayed<detail::Noop>&& noop, Action action={})
		   : vuh::Fence(std::move(noop)), Action(std::move(action)), _device(std::move(noop._device)), _result(VULKAN_HPP_NAMESPACE::Result::eSuccess)
		{}

		/// Destructor. Blocks till the undelying fence is signalled (waits forever).
		/// If the Action was not triggered previously by the call to wait() it will take place
		/// here (after the fence is triggered).
		~Delayed(){ wait(); }

		Delayed(const Delayed&) = delete;
		auto operator= (const Delayed&)-> Delayed& = delete;
		Delayed(Delayed&& other) = default;

		/// Move assignment.
		/// In case the current object owns the unsignalled fence this is going to block
		/// till that is signalled and only then proceed to taking over the move-from object.
		auto operator= (Delayed&& other) noexcept-> Delayed& {
			wait();
			static_cast<vuh::Fence&>(*this) = std::move(static_cast<vuh::Fence&>(other));
			static_cast<Action&>(*this) = std::move(static_cast<Action&>(other));
			_device = std::move(other._device);
			_result = std::move(other._result);
			return *this;
		}

		/// Blocks execution of the current thread till the underlying fence is signalled or
		/// given time period has elapsed.
		/// If the fence was signalled - triggers the Action and releases vulkan resources
		/// associated with the object (not waiting for destructor actually).
		/// If exits by the timer event - no action is taken.
		/// All is postponed till another wait() call or destructor.
		/// The function can be safely called arbitrary number of times.
		/// Or not called at all.
		auto wait(size_t period=size_t(-1) ///< time period (nanoseconds) to wait for the fence to be signalled.
		         ) noexcept-> void
		{
			if(_device){
				bool release = false;
				if (bool(vuh::Fence(*this))) {
					if (VULKAN_HPP_NAMESPACE::Result::eSuccess == _result) {
						_device->waitForFences({*this}, true, period);
						if (VULKAN_HPP_NAMESPACE::Result::eSuccess == _device->getFenceStatus(*this)) {
							release = true;
						}
					} else {
						release = true;
					}
				}

				if(release) {
					static_cast<Action &>(*this)(); // exercise action
					_device.release();
				}
			}
		}

		/// fire launch signal and wake up blocked program
		/// we use this for precise time measurement or command pool
		/// we submit some tasks to driver and wake up them when we need
		/// it's not thread safe , please call resume on the thread who create the program
		/// do'nt wait for too long time ,as we know timeout may occur about 2-3 seconds later on android
		bool resume() {
			if(bool(vuh::Event(*this))) {
				_device->setEvent(*this);
				return true;
			}
			return false;
		}

		VULKAN_HPP_NAMESPACE::Result error() const { return _result; };
		bool success() const { return VULKAN_HPP_NAMESPACE::Result::eSuccess == _result; }
		std::string error_to_string() const { return VULKAN_HPP_NAMESPACE::to_string(_result); };

	private: // data
		std::unique_ptr<Device, util::NoopDeleter<Device>> _device; ///< refers to the device owning corresponding the underlying fence.
		VULKAN_HPP_NAMESPACE::Result _result;
	}; // class Delayed
} // namespace vuh
