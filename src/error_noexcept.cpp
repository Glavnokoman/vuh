#ifdef VUH_NO_EXCEPTIONS
#include "error_noexcept.hpp"

namespace {
thread_local auto vuh_shared_error = VkResult(VK_SUCCESS);
} // namespace name

namespace vuh::error {
/// Set the error state to a given code. Should not normally be used in the client code.
auto set(VkResult err)-> void { vuh_shared_error = err; }

/// @return true if the last function left vulkan in the error state
auto err()-> bool { return vuh_shared_error != VK_SUCCESS; }

/// @return 
auto last()-> VkResult { return vuh_shared_error; }

} // namespace vuh::error
#endif // VUH_NO_EXCEPTIONS
