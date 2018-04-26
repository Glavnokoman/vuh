# Vuh. A Vulkan-based library for GPGPU computing.
Vulkan runs virtually on any modern hardware/OS combination.
It provides a low level interface to the hardware so you have all the power at your fingertips.
At the price of ridiculous amount of boilerplate.
Vuh aims to reduce the boilerplate to a reasonable minimum in most common GPGPU computing scenarios.

# Motivating Example
Draft saxpy implementation using vuh.
```c++
auto main(int argc, char const *argv[])-> int {
   auto y = std::vector<float>(128, 1.0f);
   auto x = std::vector<float>(128, 2.0f);

   auto instance = vuh::Instance();
   auto device = instance.devices()[0];                 // just get the first compute-capable device

   auto d_y = vuh::Array<float>(device, y.size());      // allocate memory on device
   d_y.from_host(y);                                    // copy from host iterable to device buffer
   auto d_x = vuh::Array<float>::from_host(x, device);  // same for x array (and a bit shorter)

   using specs_t = std::tuple<uint32_t>;                // specialization constants, here it is the workgroup size.
   using arrays_t = vuh::typelist<float*, float*>;      // array parameters to kernel
   using params_t = struct {                            // non-array parameters to kernel (push-constants), should mirror exactly corresponding structure in the shader
      uint32_t size;                                    // array size
      float a;                                          // saxpy scaling parameter
   };

   auto kernel = vuh::Kernel<specs_t, arrays_t, params_t>("shaders/saxpy.spv"); // define the kernel by linking interface and spir-v implementation
   kernel.on(device, 1)[{64}]({128, 0.1}, d_y, d_x);    // run once in queue 1, wait for completion
   d_y.to_host(y);                                      // copy data back to host

   {                                                    // kinda same but run 10 times
      auto program = kernel.on(device)                  // instantiate kernel on the device
                           .with({64})                  // set the specialization constants
                           .bind({128, 0.1}, d_y, d_x); // bind arrays and non-array parameters
      for(size_t i = 0; i < 10; ++i){
         program.run();                                 // push to default queue (0). wait for completion
      }
   }

   return 0;
}
```
with the corresponding (compute) shader part:
```glsl
layout(local_size_x_id = 0) in;             // workgroup size set with specialization constant
layout(push_constant) uniform Parameters {  // push constants
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
Pity it does not work yet like this.

# Build & Install
## Dependencies
- C++14 compliant compiler
- Vulkan
- Vulkan-hpp
- Glslang (optional, but very recommended)
- CMake (build-only)
- Catch2 (optional, build-only)
- sltbench (optional, build-only)

# Use
