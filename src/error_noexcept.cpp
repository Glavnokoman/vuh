#ifdef VUH_NO_EXCEPTIONS
#include "error_noexcept.hpp"

namespace {
thread_local auto vuh_shared_error = VkResult(VK_SUCCESS);
} // namespace name

namespace vuh::error {
///
auto set(VkResult err)-> void { vuh_shared_error = err; }

///
auto err()-> bool { return vuh_shared_error != VK_SUCCESS; }

///
auto last()-> VkResult { return vuh_shared_error; }

} // namespace vuh::error
#endif // VUH_NO_EXCEPTIONS
