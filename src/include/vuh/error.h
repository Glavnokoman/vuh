#pragma once

#include "vuh/core/core.hpp"

#ifndef VULKAN_HPP_NO_EXCEPTIONS

namespace vuh {
	/// Exception indicating that memory with requested properties
	/// has not been found on a device.
	class NoSuitableMemoryFound: public vhn::OutOfDeviceMemoryError {
	public:
		NoSuitableMemoryFound(const std::string& msg);
		NoSuitableMemoryFound(const char* msg);
	};

	/// Exception indicating failure to read a file.
	class FileReadFailure: public std::runtime_error {
	public:
		FileReadFailure(const std::string& msg);
		FileReadFailure(const char* msg);
	};
} // namespace vuh

#endif
