#pragma once

#include "vuh/core/core.hpp"
#include <vuh/device.h>
#include <vuh/resource.hpp>

namespace vuh {
	/// Class used for synchronization with host.
	/// vulkan fence  
	class Fence : public vhn::Fence {
	public:
		Fence() : vhn::Fence() {

		}

		explicit Fence(vuh::Device& device, bool signaled=false)
				: _device(&device)
				, _result(vhn::Result::eSuccess)
		{
#if VK_HEADER_VERSION >= 70 // ExternalFenceHandleTypeFlagBits define changed from VK_HEADER_VERSION(70)
        #ifdef VK_USE_PLATFORM_WIN32_KHR
            // which one is correct on windows ?
            vhn::ExportFenceCreateInfoKHR efci(vhn::ExternalFenceHandleTypeFlagBits::eOpaqueWin32);
            //VULKAN_HPP_NAMESPACE::ExportFenceWin32HandleInfoKHR efci;
        #elif VK_USE_PLATFORM_ANDROID_KHR // current android only support VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT
            vhn::ExportFenceCreateInfoKHR efci(vhn::ExternalFenceHandleTypeFlagBits::eSyncFd);
        #else
            vhn::ExportFenceCreateInfoKHR efci(vhn::ExternalFenceHandleTypeFlagBits::eOpaqueFd);
        #endif
#else									
	#ifdef VK_USE_PLATFORM_WIN32_KHR
			// which one is correct on windows ?
			vhn::ExportFenceCreateInfoKHR efci(vhn::ExternalFenceHandleTypeFlagBits::eOpaqueWin32);
			//vhn::ExportFenceWin32HandleInfoKHR efci;
	#elif VK_USE_PLATFORM_ANDROID_KHR // current android only support VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT
			vhn::ExportFenceCreateInfoKHR efci(vhn::ExternalFenceHandleTypeFlagBitsKHR::eSyncFd);
	#else
			vhn::ExportFenceCreateInfoKHR efci(vhn::ExternalFenceHandleTypeFlagBitsKHR::eOpaqueFd);
	#endif
#endif
			vhn::FenceCreateInfo fci;
			if (signaled) {
				fci.setFlags(vhn::FenceCreateFlagBits::eSignaled);
			}
			if(_device->supportFenceFd()) {
				fci.setPNext(&efci);
			}
			auto fen = _device->createFence(fci);
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		    _result = fen.result;
		    VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _result);
		    static_cast<vhn::Fence&>(*this) = std::move(fen.value);
#else
			static_cast<vhn::Fence&>(*this) = std::move(fen);
#endif			
		}
		
		explicit Fence(const vuh::Fence& fence)
		{
			static_cast<vhn::Fence&>(*this) = std::move(fence);
			_device = std::move(const_cast<vuh::Fence&>(fence)._device);
			_result = std::move(fence._result);
		}

		~Fence()
		{
			if(success()) {
				_device->destroyFence(*this);
			}
			_device.release();
		}

		auto operator= (const vuh::Fence&)-> vuh::Fence& = delete;
		Fence(vuh::Fence&& other) = default;

		/// Move assignment.
		/// In case the current object owns the unsignalled fence this is going to block
		/// till that is signalled and only then proceed to taking over the move-from object.
		auto operator= (vuh::Fence&& other) noexcept-> vuh::Fence& {
			static_cast<vhn::Fence&>(*this) = std::move(other);
			_device = std::move(other._device);
			_result = std::move(other._result);
			return *this;
		}

		explicit operator bool() const {
			return success();
		}

		VULKAN_HPP_TYPESAFE_EXPLICIT operator VkFence () const
		{
			return VkFence(static_cast<const vhn::Fence&>(*this));
		}

		/// if fenceFd() is called ,do'nt use this function wait, this function will blocked and never return
		/// please call epoll_wait/select/poll wait for fd's signal
		auto wait(size_t period=size_t(-1))-> bool {
			if(success()) {
				_device->waitForFences({*this}, true, period);
				auto result = _device->getFenceStatus(*this);
				return (vhn::Result::eSuccess == result);
			}
			return false;
		}
		
		/// if fenceFd is support, we can use epoll or select wait for fence complete
		bool supportFenceFd() {
			return _device->supportFenceFd();
		}		
	
#ifdef VK_USE_PLATFORM_WIN32_KHR
        auto fenceFd(HANDLE& fd)-> vhn::Result {
			vhn::FenceGetWin32HandleInfoKHR info(*this);
			auto res = _device->getFenceWin32HandleKHR(info);
#else
		auto fenceFd(int& fd)-> vhn::Result {
			// following https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html
			// current android only support VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT
			// If handleType is VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT,
			// the special value -1 for fd is treated like a valid sync file descriptor referring to an object that has already signaled.
			// The import operation will succeed and the VkFence will have a temporarily imported payload as if a valid file descriptor had been provided.
		#ifdef VK_USE_PLATFORM_ANDROID_KHR /* Android need dynamic load KHR extension */
        	#if VK_HEADER_VERSION >= 70 // ExternalFenceHandleTypeFlagBits define changed from VK_HEADER_VERSION(70)
				vhn::FenceGetFdInfoKHR info(*this,vhn::ExternalFenceHandleTypeFlagBits::eSyncFd);
            #else
				vhn::FenceGetFdInfoKHR info(*this,vhn::ExternalFenceHandleTypeFlagBitsKHR::eSyncFd);
            #endif
			auto res = _device->getFenceFdKHR(info,vhn::DispatchLoaderDynamic(vhn::Instance(_device->instance()),*_device));
        #else
			#if VK_HEADER_VERSION >= 70 // ExternalFenceHandleTypeFlagBits define changed from VK_HEADER_VERSION(70)
				vhn::FenceGetFdInfoKHR info(*this,vhn::ExternalFenceHandleTypeFlagBits::eOpaqueFd);
        	#else
				vhn::FenceGetFdInfoKHR info(*this,vhn::ExternalFenceHandleTypeFlagBitsKHR::eOpaqueFd);
        	#endif
			auto res = _device->getFenceFdKHR(info);
        #endif
#endif
#ifdef VULKAN_HPP_NO_EXCEPTIONS
			fd = res.value;
			return res.result;
#else
			fd = res;
			return vhn::Result::eSuccess;
#endif
		}

		vhn::Result error() const { return _result; };
		bool success() const { return (vhn::Result::eSuccess == _result) && bool(static_cast<const vhn::Fence&>(*this)) && (nullptr != _device) ; }
		std::string error_to_string() const { return vhn::to_string(_result); };

	private: // data
		std::unique_ptr<vuh::Device, util::NoopDeleter<vuh::Device>> _device; ///< refers to the device owning corresponding the underlying fence.
		vhn::Result _result;
	};	
} // namespace vuh