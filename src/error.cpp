#include "vuh/error.h"

namespace vuh {
	/// Constructs the exception object with explanatory string.
	NoSuitableMemoryFound::NoSuitableMemoryFound(const std::string& message)
	   : vk::OutOfDeviceMemoryError(message)
	{}

	/// Constructs the exception object with explanatory string.
	NoSuitableMemoryFound::NoSuitableMemoryFound(const char* message)
	   : vk::OutOfDeviceMemoryError(message)
	{}

	/// Constructs the exception object with explanatory string.
	FileReadFailure::FileReadFailure(const std::string& message)
	   : std::runtime_error(message)
	{}

	/// Constructs the exception object with explanatory string.
	FileReadFailure::FileReadFailure(const char* message)
	   : std::runtime_error(message)
	{}

} // namespace vuh
