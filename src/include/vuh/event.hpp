#pragma once

#include "vuh/core/core.hpp"
#include <vuh/device.h>
#include <vuh/resource.hpp>

namespace vuh {
	/// vulkan Event  
	class Event : public vhn::Event, public vuh::VuhBasic {
	public:
		Event() : vhn::Event() {

		}

		explicit Event(vuh::Device& device)
				: _device(&device)
		{
			auto ev = _device->createEvent(vhn::EventCreateInfo());
#ifdef VULKAN_HPP_NO_EXCEPTIONS
			_res = ev.result;
			VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _res);
			static_cast<vhn::Event&>(*this) = std::move(ev.value);
#else
			static_cast<vhn::Event&>(*this) = std::move(ev);
#endif	
		}
		
		explicit Event(const vuh::Event& ev)
		{
			static_cast<vhn::Event&>(*this) = std::move(ev);
			_device = std::move(const_cast<vuh::Event&>(ev)._device);
			_res = std::move(ev._res);
		}

		~Event()
		{
			if(success()) {
				_device->destroyEvent(*this);
			}
			_device.release();
		}

		auto operator= (const vuh::Event&)-> vuh::Event& = delete;
		Event(vuh::Event&& other) = default;

		/// Move assignment.
		/// In case the current object owns the unsignalled fence this is going to block
		/// till that is signalled and only then proceed to taking over the move-from object.
		auto operator= (vuh::Event&& other) noexcept-> vuh::Event& {
			static_cast<vhn::Event&>(*this) = std::move(other);
			_device = std::move(other._device);
			_res = std::move(other._res);
			return *this;
		}

		explicit operator bool() const {
			return success();
		}

		VULKAN_HPP_TYPESAFE_EXPLICIT operator VkEvent() const
		{
			return VkEvent(static_cast<const vhn::Event&>(*this));
		}

		bool setEvent() const {
			if(success()) {
				_device->setEvent(*this);
				return true;
			}
			return false;
		}

		bool success() const { return VuhBasic::success() && bool(static_cast<const vhn::Event&>(*this)) && (nullptr != _device); }
	private: // data
		std::unique_ptr<vuh::Device, util::NoopDeleter<vuh::Device>> _device; ///< refers to the device owning corresponding the underlying fence.
	};	
} // namespace vuh