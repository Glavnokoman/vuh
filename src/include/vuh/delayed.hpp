#pragma once

#include <vulkan/vulkan.hpp>
#include <vuh/device.h>
#include <vuh/resource.hpp>

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
	class Delayed: public vk::Fence, vk::Event, private Action {
		template<class> friend class Delayed;
	public:
		/// Constructor. Takes ownership of the fence.
		/// It is assumed that the fence belongs to the same device that is passed together with it.
		Delayed(vk::Fence fence, vuh::Device& device, Action action={})
		   : vk::Fence(fence)
		   , Action(std::move(action))
		   , _device(&device)
		   , _result(vk::Result::eSuccess)
		{}

		explicit Delayed(vk::Fence fence, vuh::Device& device,vk::Result result, Action action={})
				: vk::Fence(fence)
				, Action(std::move(action))
				, _device(&device)
				, _result(result)
		{}

        explicit Delayed(vk::Fence fence, vk::Event event, vuh::Device& device, Action action={})
                : vk::Fence(fence)
				, vk::Event(event)
                , Action(std::move(action))
                , _device(&device)
                , _result(vk::Result::eSuccess)
        {}

		/// Constructs for VULKAN_HPP_NO_EXCEPTIONS
		explicit Delayed(vuh::Device& device, vk::Result result, vk::Event event, Action action={})
				: vk::Event(event)
				, Action(std::move(action))
				, _device(&device)
				, _result(result)
		{}

		/// Constructor. Creates the fence in a signalled state.
		explicit Delayed(vuh::Device& device, Action action={})
				: Action(std::move(action))
				, _device(&device)
				, _result(vk::Result::eSuccess)
		{
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		    auto fen = device.createFence({vk::FenceCreateFlagBits::eSignaled});
		    _result = fen.result;
		    VULKAN_HPP_ASSERT(vk::Result::eSuccess == _result);
		    vk::Fence(*this) = fen.value;
#else
			vk::Fence(*this) = device.createFence({vk::FenceCreateFlagBits::eSignaled});
#endif
		}


		/// Constructs from the object of Delayed<Noop> and inherits the state of that.
		/// Takes over the undelying fence ownership.
		/// Mostly substitute its own action in place of Noop.
		explicit Delayed(Delayed<detail::Noop>&& noop, Action action={})
		   : vk::Fence(std::move(noop)), Action(std::move(action)), _device(std::move(noop._device)), _result(vk::Result::eSuccess)
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
			static_cast<vk::Fence&>(*this) = std::move(static_cast<vk::Fence&>(other));
			static_cast<Action&>(*this) = std::move(static_cast<Action&>(other));
			_device = std::move(other._device);
			_result = other._result;
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
				if (bool(vk::Fence(*this))) {
					if (vk::Result::eSuccess == _result) {
						_device->waitForFences({*this}, true, period);
						if (vk::Result::eSuccess == _device->getFenceStatus(*this)) {
							_device->destroyFence(*this);
							release = true;
						}
					} else {
						_device->destroyFence(*this);
						release = true;
					}
				}

				if(release) {
					if (bool(vk::Event(*this))) {
						_device->destroyEvent(*this);
					}
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
			if(bool(vk::Event(*this))) {
				_device->setEvent(*this);
				return true;
			}
			return false;
		}

		vk::Result error() const { return _result; };
		bool success() const { return vk::Result::eSuccess == _result; }
		std::string error_to_string() const { return vk::to_string(_result); };

	private: // data
		std::unique_ptr<Device, util::NoopDeleter<Device>> _device; ///< refers to the device owning corresponding the underlying fence.
		vk::Result _result;
	}; // class Delayed

	/// Delayed No-Action. Just a synchronization point.
	using Fence = Delayed<detail::Noop>;
} // namespace vuh
