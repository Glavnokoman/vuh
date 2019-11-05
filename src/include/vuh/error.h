#pragma once

#include "vuh/core/core.hpp"

#ifndef VULKAN_HPP_NO_EXCEPTIONS

namespace vuh {
	/// Exception indicating that memory with requested properties
	/// has not been found on a device.
	class NoSuitableMemoryFound: public vhn::OutOfDeviceMemoryError {
	public:
		NoSuitableMemoryFound(const std::string& message);
		NoSuitableMemoryFound(const char* message);
	};

	/// Exception indicating failure to read a file.
	class FileReadFailure: public std::runtime_error {
	public:
		FileReadFailure(const std::string& message);
		FileReadFailure(const char* message);
	};
} // namespace vuh

#endif
