# vuh library version 2.0
vuh 2.0 is a major rewrite of a library undertaken due to several major design issues present in the previous versions of the library.
In particular this rewrite emphasizes:
- follow the normal vulkan workflow more closely
- all wrapper classes should allow for easy assimilating the underlying Vulkan resource
- option to build with exceptions disabled
- no dependency on vulkan.hpp
- compliance to vulkan 1.0 (nice to have an option to use later standards functionality when available)
- more consistent copy routines
- better testing

target audience
 - embedded
 - android/iOS cross-platform development
 - more general cross-platform development including Apple-made targets



