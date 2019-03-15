#pragma once

#include <vulkan/vulkan.hpp>
#include <vuh/device.h>
#include <vuh/resource.hpp>

namespace vuh {
	/// vulkan Event  
	class Event : public VULKAN_HPP_NAMESPACE::Event {
	public:
		Event() : VULKAN_HPP_NAMESPACE::Event() {

		}

		explicit Event(vuh::Device& device)
				: _device(&device)
				, _result(VULKAN_HPP_NAMESPACE::Result::eSuccess)
		{
			auto ev = _device->createEvent(VULKAN_HPP_NAMESPACE::EventCreateInfo());
#ifdef VULKAN_HPP_NO_EXCEPTIONS
			_result = ev.result;
			VULKAN_HPP_ASSERT(VULKAN_HPP_NAMESPACE::Result::eSuccess == _result);
			static_cast<VULKAN_HPP_NAMESPACE::Event&>(*this) = std::move(ev.value);
#else
			static_cast<VULKAN_HPP_NAMESPACE::Event&>(*this) = std::move(ev);
#endif	
		}
		
		explicit Event(const vuh::Event& ev)
		{
			static_cast<VULKAN_HPP_NAMESPACE::Event&>(*this) = std::move(ev);
			_device = std::move(const_cast<vuh::Event&>(ev)._device);
			_result = std::move(ev._result);
		}

		~Event()
		{
			if(success()) {
				_device->destroyEvent(*this);
			}
		}

		auto operator= (const vuh::Event&)-> vuh::Event& = delete;
		Event(vuh::Event&& other) = default;

		/// Move assignment.
		/// In case the current object owns the unsignalled fence this is going to block
		/// till that is signalled and only then proceed to taking over the move-from object.
		auto operator= (vuh::Event&& other) noexcept-> vuh::Event& {
			static_cast<VULKAN_HPP_NAMESPACE::Event&>(*this) = std::move(other);
			_device = std::move(other._device);
			_result = std::move(other._result);
			return *this;
		}

		explicit operator bool() const {
			return success();
		}
		
		VULKAN_HPP_NAMESPACE::Result error() const { return _result; };
		bool success() const { return (VULKAN_HPP_NAMESPACE::Result::eSuccess == _result) && bool(VULKAN_HPP_NAMESPACE::Event(*this)); }
		std::string error_to_string() const { return VULKAN_HPP_NAMESPACE::to_string(_result); };			

	private: // data
		std::unique_ptr<vuh::Device, util::NoopDeleter<vuh::Device>> _device; ///< refers to the device owning corresponding the underlying fence.
		VULKAN_HPP_NAMESPACE::Result _result;
	};	
} // namespace vuh