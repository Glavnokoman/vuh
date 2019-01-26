# Features NOT to come
This is to list the non-goals or things that are simply not achievable with a current approach
- Full-blown Vulkan wrapper - this is first and foremost GPGPU helper
- Reimplement some better-known GPGPU API (like Cuda or OpenCL) - the point is to provide an API which better reflects (and connects to) an underlying Vulkan structure and is more human friendly at the same time
- Library of compute shader primitives - worth of a separate project
- Dynamic parallelism - I do not see how it is doable in the Vulkan-SPIR-V framework
