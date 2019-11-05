#pragma once

#include "vuh/core/core.hpp"
#include <vuh/device.h>
#include <vuh/resource.hpp>

namespace vuh {
	/// vulkan Event  
	class Event : public vhn::Event {
	public:
		Event() : vhn::Event() {

		}

		explicit Event(vuh::Device& device)
				: _device(&device)
				, _result(vhn::Result::eSuccess)
		{
			auto ev = _device->createEvent(vhn::EventCreateInfo());
#ifdef VULKAN_HPP_NO_EXCEPTIONS
			_result = ev.result;
			VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _result);
			static_cast<vhn::Event&>(*this) = std::move(ev.value);
#else
			static_cast<vhn::Event&>(*this) = std::move(ev);
#endif	
		}
		
		explicit Event(const vuh::Event& ev)
		{
			static_cast<vhn::Event&>(*this) = std::move(ev);
			_device = std::move(const_cast<vuh::Event&>(ev)._device);
			_result = std::move(ev._result);
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
			_result = std::move(other._result);
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

		vhn::Result error() const { return _result; };
		bool success() const { return (vhn::Result::eSuccess == _result) && bool(static_cast<const vhn::Event&>(*this)) && (nullptr != _device); }
		std::string error_to_string() const { return vhn::to_string(_result); };

	private: // data
		std::unique_ptr<vuh::Device, util::NoopDeleter<vuh::Device>> _device; ///< refers to the device owning corresponding the underlying fence.
		vhn::Result _result;
	};	
} // namespace vuh