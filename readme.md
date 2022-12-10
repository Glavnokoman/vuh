# Watch out!
I do not have time or interest any more to further maintain or develop this.
I will lazily review the bug-fix requests but no(t much) effort is made to maintain the code quality or style and no new releases will be made.

# Vuh. A Vulkan-based GPGPU computing framework.
[![Build Status](https://travis-ci.org/Glavnokoman/vuh.svg?branch=master)](https://travis-ci.org/Glavnokoman/vuh)

Vulkan is the most widely supported GPU programming API on modern hardware/OS.
It allows to write truly portable and performant GPU accelerated code that would run on iOS, Android, Linux, Windows, macOS... NVidia, AMD, Intel, Adreno, Mali... whatever.
At the price of ridiculous amount of boilerplate.
Vuh aims to reduce the boilerplate to (a reasonable) minimum in most common GPGPU computing scenarios.
The ultimate goal is to beat OpenCL in usability, portability and performance.

# Motivating Example
saxpy implementation using vuh.
```c++
auto main()-> int {
   auto y = std::vector<float>(128, 1.0f);
   auto x = std::vector<float>(128, 2.0f);

   auto instance = vuh::Instance();
   auto device = instance.devices().at(0);    // just get the first available device

   auto d_y = vuh::Array<float>(device, y);   // create device arrays and copy data
   auto d_x = vuh::Array<float>(device, x);

   using Specs = vuh::typelist<uint32_t>;     // shader specialization constants interface
   struct Params{uint32_t size; float a;};    // shader push-constants interface
   auto program = vuh::Program<Specs, Params>(device, "saxpy.spv"); // load shader
   program.grid(128/64).spec(64)({128, 0.1}, d_y, d_x); // run once, wait for completion

   d_y.toHost(begin(y));                      // copy data back to host

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

# Features
- storage buffers as ```vuh::Array<T>```
   + allocated in device-local, host-visible or device-local-host-visible memories
   + data exchange with host incl. hidden staging buffers
- computation kernels as ```vuh::Program```
   + buffers binding (passing arbitrary number of array parameters)
   + specialization constants (to set workgroup dimensions, etc...)
   + push-constants (to pass small data (<= 128 Bytes), like task dimensions etc...)
   + whatever compute shaders support, shared memory, etc...
- asynchronous data transfer and kernel execution with host-side synchronization
- multiple device support
- [yet to come...](doc/features_to_come.md)
- [not ever coming...](doc/features_not_to_come.md)

# Usage
- [Build & Install](doc/build_install.md)
- [Tutorial](doc/tutorial.md)
- [Examples](doc/examples/examples.md)
