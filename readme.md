# Vuh. A Vulkan-based GPGPU computing framework.
[![Build Status](https://travis-ci.org/Glavnokoman/vuh.svg?branch=master)](https://travis-ci.org/Glavnokoman/vuh)

Vulkan runs virtually on any modern hardware/OS combination.
It allows you to write GPU accelerated code once, and run it on iOS, Android, Linux, Windows, macOS...
NVidia, Radeons, Intel, Adreno, Mali... whatever.
Vulkan provides a very low level interface so you have access to all the power hidden in the GPU.
At the price of ridiculous amount of boilerplate.
Vuh aims to reduce the boilerplate to (a reasonable) minimum in most common GPGPU computing scenarios.

# Motivating Example
saxpy implementation using vuh.
```c++
auto main()-> int {
   auto y = std::vector<float>(128, 1.0f);
   auto x = std::vector<float>(128, 2.0f);

   auto instance = vuh::Instance();
   auto device = instance.devices().at(0);              // just get the first available device

   auto d_y = vuh::Array<float>::fromHost(device, y);   // create device arrays and copy data
   auto d_x = vuh::Array<float>::fromHost(device, x);

   using Specs = vuh::typelist<uint32_t>;               // shader specialization constants interface
   struct Params{uint32_t size; float a;};              // shader push-constants interface
   auto program = vuh::Program<Specs, Params>(device, "saxpy.spv"); // load spir-v shader code
   program.grid(128/64).spec(64)({128, 0.1}, d_y, d_x); // run once, wait for completion

   d_y.toHost(y);                                       // copy data back to host

   return 0;
}
```
and the corresponding kernel (glsl compute shader) code:
```glsl
layout(local_size_x_id = 0) in;             // workgroup size (set with .spec(64) on C++ side)
layout(push_constant) uniform Parameters {  // push constants (set with {128, 0.1} on C++ side)
   uint size;                               // array size
   float a;                                 // scaling parameter
} params;

layout(std430, binding = 0) buffer lay0 { float arr_y[]; }; // array parameters
layout(std430, binding = 1) buffer lay1 { float arr_x[]; };

void main(){
   const uint id = gl_GlobalInvocationID.x; // current offset
   if(params.size <= id){                   // drop threads outside the buffer
      return;
   }
   arr_y[id] += params.a*arr_x[id];         // saxpy
}
```

# Features (so far)
- device arrays, sync data exchange with host
- specialization constants (to set workgroup dimensions, etc...)
- push-constants (to pass small data (<= 128 Bytes), like task dimensions etc...)
- layout bindings (to pass array parameters)
- sync kernel execution

# Build & Install
## Dependencies
- C++14 compliant compiler
- [Vulkan-headers](https://github.com/KhronosGroup/Vulkan-Headers)
- [Vulkan-hpp](https://github.com/KhronosGroup/Vulkan-Hpp)
- [Glslang](https://github.com/KhronosGroup/glslang) (optional (but very recommended))
- [CMake](https://cmake.org/download/) (build-only)
- [Catch2](https://github.com/catchorg/Catch2) (optional, build-only)
- [sltbench](https://github.com/ivafanas/sltbench) (optional, build-only)

# Use
