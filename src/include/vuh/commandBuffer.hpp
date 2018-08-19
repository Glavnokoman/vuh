#pragma once

#include "device.h"
#include "resource.hpp"

namespace vuh {
	namespace detail {
		/// Command buffer data packed with allocation and deallocation methods.
		struct _CmdBuffer {
			/// Constructor. Creates the new command buffer on a provided device and manages its resources.
			_CmdBuffer(vuh::Device& device): device(&device){
				auto bufferAI = vk::CommandBufferAllocateInfo(device.transferCmdPool()
				                                              , vk::CommandBufferLevel::ePrimary, 1);
				cmd_buffer = device.allocateCommandBuffers(bufferAI)[0];
			}

			/// Constructor. Takes ownership over the provided buffer.
			/// @pre buffer should belong to the provided device. No check is made even in a debug build.
			_CmdBuffer(vuh::Device& device, vk::CommandBuffer buffer)
			   : cmd_buffer(buffer), device(&device)
			{}

			/// Release the buffer resources
			auto release() noexcept-> void {
				if(device){
					device->freeCommandBuffers(device->transferCmdPool(), 1, &cmd_buffer);
				}
			}
		public: // data
			vk::CommandBuffer cmd_buffer; ///< command buffer managed by this wrapper class
			std::unique_ptr<vuh::Device, util::NoopDeleter<vuh::Device>> device; ///< device holding the buffer
		}; // struct CmdBuffer
	} // namespace detail

	using CmdBuffer = util::Resource<detail::_CmdBuffer>;
} // namespace vuh
