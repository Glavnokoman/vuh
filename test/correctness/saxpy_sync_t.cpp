#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <vector>
#include <tuple>
#include <cstdint>

auto main(int /*argc*/, char const */*argv*/[])-> int {
   auto y = std::vector<float>(128, 1.0f);
   auto x = std::vector<float>(128, 2.0f);
   
   auto instance = vuh::Instance();
   auto device = instance.devices()[0];                  // get the first available device
   
   auto d_y = vuh::Array<float>(device, y.size());       // allocate memory on device
   d_y.fromHost(y);                                      // synchronous copy from host iterable to device buffer
   auto d_x = vuh::Array<float>::fromHost(x, device);    // same for x array
   
   using specs_t = std::tuple<uint32_t>;                 // specialization constants, here it is the workgroup size.
   using array_params_t = vuh::typelist<float, const float>; // array parameters to kernel
   using params_t = struct {                             // non-array parameters to kernel (push-constants), should mirror exactly corresponding structure in the shader
      uint32_t size;                                     // array size
      float a;                                           // saxpy scaling parameter
   };

   auto kernel = vuh::Kernel<specs_t, array_params_t, params_t>("shaders/saxpy.spv"); // define the kernel by linking interface and spir-v implementation
   kernel.on(device).batch(128/64)(specs_t{64}, {128, 0.1}, d_y, d_x);  // run once, wait for completion
   
   {
      auto instance = kernel.on(device)                  // instantiate kernel on the device
                            .batch(128/64)               // set number of wrokgroups to run
                            .bind(specs_t{64})           // set the specialization constants. Not supposed to change frequently.
                            .bind({128, 0.1}, d_y, d_x); // bind arrays and non-array parameters
      for(size_t i = 0; i < 10; ++i){
         instance.run();                                 // run 10 times
      }
   }

   d_y.toHost(y);                                       // copy data back to host
   
   return 0;
}
