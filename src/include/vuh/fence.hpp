#pragma once

#include <vulkan/vulkan.hpp>
#include <vuh/device.h>
#include <vuh/resource.hpp>

namespace vuh {
	/// Class used for synchronization with host.
	/// vulkan fence  
	class Fence : public VULKAN_HPP_NAMESPACE::Fence {
	public:
		explicit Fence(vuh::Device& device)
				: _device(&device)
				, _result(VULKAN_HPP_NAMESPACE::Result::eSuccess)
		{
#if VK_HEADER_VERSION >= 70 // ExternalFenceHandleTypeFlagBits define changed from VK_HEADER_VERSION(70)
	#ifdef VK_USE_PLATFORM_WIN32_KHR
		VULKAN_HPP_NAMESPACE::ExportFenceCreateInfoKHR efci(VULKAN_HPP_NAMESPACE::ExternalFenceHandleTypeFlagBits::eOpaqueWin32);
	#elif VK_USE_PLATFORM_ANDROID_KHR // current android only support VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT
		VULKAN_HPP_NAMESPACE::ExportFenceCreateInfoKHR efci(VULKAN_HPP_NAMESPACE::ExternalFenceHandleTypeFlagBits::eSyncFd);
	#else
		VULKAN_HPP_NAMESPACE::ExportFenceCreateInfoKHR efci(VULKAN_HPP_NAMESPACE::ExternalFenceHandleTypeFlagBits::eOpaqueFd);
	#endif
#else									
	#ifdef VK_USE_PLATFORM_WIN32_KHR
			VULKAN_HPP_NAMESPACE::ExportFenceCreateInfoKHR efci(VULKAN_HPP_NAMESPACE::ExternalFenceHandleTypeFlagBitsKHR::eOpaqueWin32);
	#elif VK_USE_PLATFORM_ANDROID_KHR // current android only support VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT
			VULKAN_HPP_NAMESPACE::ExportFenceCreateInfoKHR efci(VULKAN_HPP_NAMESPACE::ExternalFenceHandleTypeFlagBitsKHR::eSyncFd);
	#else
			VULKAN_HPP_NAMESPACE::ExportFenceCreateInfoKHR efci(VULKAN_HPP_NAMESPACE::ExternalFenceHandleTypeFlagBitsKHR::eOpaqueFd);
	#endif
#endif			
			VULKAN_HPP_NAMESPACE::FenceCreateInfo fci = VULKAN_HPP_NAMESPACE::FenceCreateInfo(VULKAN_HPP_NAMESPACE::FenceCreateFlagBits::eSignaled);
			if(_device->supportFenceFd()) {
				fci.setPNext(&efci);
			}
			auto fen = _device->createFence(fci);
#ifdef VULKAN_HPP_NO_EXCEPTIONS
		    _result = fen.result;
		    VULKAN_HPP_ASSERT(VULKAN_HPP_NAMESPACE::Result::eSuccess == _result);
		    static_cast<VULKAN_HPP_NAMESPACE::Fence&>(*this) = std::move(fen.value);
#else
			static_cast<VULKAN_HPP_NAMESPACE::Fence&>(*this) = std::move(fen);
#endif			
		}
		
		explicit Fence(const vuh::Fence& fence)
		{
			static_cast<VULKAN_HPP_NAMESPACE::Fence&>(*this) = std::move(fence);
			_device = std::move(const_cast<vuh::Fence&>(fence)._device);
			_result = std::move(fence._result);
		}

		~Fence()
		{
			if(success()) {
				_device->destroyFence(*this);
			}
		}

		auto operator= (const vuh::Fence&)-> vuh::Fence& = delete;
		Fence(vuh::Fence&& other) = default;

		/// Move assignment.
		/// In case the current object owns the unsignalled fence this is going to block
		/// till that is signalled and only then proceed to taking over the move-from object.
		auto operator= (vuh::Fence&& other) noexcept-> vuh::Fence& {
			static_cast<VULKAN_HPP_NAMESPACE::Fence&>(*this) = std::move(other);
			_device = std::move(other._device);
			_result = std::move(other._result);
			return *this;
		}

		explicit operator bool() const {
			return success();
		}
		
		// if fenceFd is support, we can use epoll or select wait for fence complete
		bool supportFenceFd() {
			return _device->supportFenceFd();
		}		
	
#ifdef VK_USE_PLATFORM_WIN32_KHR
        auto fenceFd(HANDLE& fd)-> VULKAN_HPP_NAMESPACE::Result {
			VULKAN_HPP_NAMESPACE::FenceGetWin32HandleInfoKHR info(*this);
			auto res = _device->getFenceWin32HandleKHR(info);
#else
		auto fenceFd(int& fd)-> VULKAN_HPP_NAMESPACE::Result {
			// following https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html
			// current android only support VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT
			// If handleType is VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT,
			// the special value -1 for fd is treated like a valid sync file descriptor referring to an object that has already signaled.
			// The import operation will succeed and the VkFence will have a temporarily imported payload as if a valid file descriptor had been provided.
		#ifdef VK_USE_PLATFORM_ANDROID_KHR /* Android need dynamic load KHR extension */
        	#if VK_HEADER_VERSION >= 70 // ExternalFenceHandleTypeFlagBits define changed from VK_HEADER_VERSION(70)
				VULKAN_HPP_NAMESPACE::FenceGetFdInfoKHR info(*this,VULKAN_HPP_NAMESPACE::ExternalFenceHandleTypeFlagBits::eSyncFd);
            #else
				VULKAN_HPP_NAMESPACE::FenceGetFdInfoKHR info(*this,VULKAN_HPP_NAMESPACE::ExternalFenceHandleTypeFlagBitsKHR::eSyncFd);
            #endif
			auto res = _device->getFenceFdKHR(info,vk::DispatchLoaderDynamic(vk::Instance(_device->instance()),*_device));
        #else
			#if VK_HEADER_VERSION >= 70 // ExternalFenceHandleTypeFlagBits define changed from VK_HEADER_VERSION(70)
				VULKAN_HPP_NAMESPACE::FenceGetFdInfoKHR info(*this,VULKAN_HPP_NAMESPACE::ExternalFenceHandleTypeFlagBits::eOpaqueFd);
        	#else
				VULKAN_HPP_NAMESPACE::FenceGetFdInfoKHR info(*this,VULKAN_HPP_NAMESPACE::ExternalFenceHandleTypeFlagBitsKHR::eOpaqueFd);
        	#endif
			auto res = _device->getFenceFdKHR(info);
        #endif
#endif
#ifdef VULKAN_HPP_NO_EXCEPTIONS
			fd = res.value;
			return res.result;
#else
			fd = res;
			return VULKAN_HPP_NAMESPACE::Result::eSuccess;
#endif
		}
		
		VULKAN_HPP_NAMESPACE::Result error() const { return _result; };
		bool success() const { return (VULKAN_HPP_NAMESPACE::Result::eSuccess == _result) && bool(VULKAN_HPP_NAMESPACE::Fence(*this)) && (nullptr != _device) ; }
		std::string error_to_string() const { return VULKAN_HPP_NAMESPACE::to_string(_result); };			

	private: // data
		std::unique_ptr<vuh::Device, util::NoopDeleter<vuh::Device>> _device; ///< refers to the device owning corresponding the underlying fence.
		VULKAN_HPP_NAMESPACE::Result _result;
	};	
} // namespace vuh