#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <vector>

auto main(int argc, char const *argv[])-> int {
   auto y = std::vector<float>(100, 1.0f);
   auto x = std::vector<float>(100, 2.0f);

   auto instance = vuh::instance();
   auto device = instance.devices()[0];                  // get the first available device

   auto d_y = vuh::Array<float>(device, y.size());       // allocate memory on device
   d_y.from_host(y);                                     // synchronous copy from host iterable to device buffer
   auto d_x = vuh::Array<float>::from_host(x, device);   // same for x array

   using specs_t = std::tuple<uint32_t>;                 // specialization constants, here it is the workgroup size.
   using array_params_t = vuh::typelist<float*, float*>; // array parameters to kernel
   using params_t = struct {                             // non-array parameters to kernel (push-constants), should mirror exactly corresponding structure in the shader
      uint32_t size;                                     // array size
      float a;                                           // saxpy scaling parameter
   };

   auto kernel = vuh::Kernel<specs_t, array_params_t, params_t>("shaders/saxpy.spv"); // define the kernel by linking interface and spir-v implementation
   {
      auto instance = kernel.on(device);                 // instantiate kernel on the device
      instance.specialize({64});                         // set the specialization constants. Not supposed to change frequently.
      instance.bind({128, 0.1}, d_y, d_x);               // bind arrays and non-array parameters
      instance.run_in(0);                                // push to queue 0 in compute queue family, wait for result
   }
   kernel.on(device, {64})({128, 0.1}, d_y, d_x);        // same but shorter

   d_y.to_host(y);                                       // copy data back to host

   return 0;
}
