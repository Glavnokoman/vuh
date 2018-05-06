#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <vector>
#include <tuple>
#include <cstdint>

auto main(int /*argc*/, char const */*argv*/[])-> int {
	auto y = std::vector<float>(128, 1.0f);
	auto x = std::vector<float>(128, 2.0f);

	auto instance = vuh::Instance();
	auto device = instance.devices()[0];               // just get the first compute-capable device

	auto d_y = vuh::Array<float>::fromHost(device, y); // allocate memory on device and copy data from host
	auto d_x = vuh::Array<float>::fromHost(device, x); // same for x

	using specs_t = std::tuple<uint32_t>;              // specialization constants, here it is the workgroup size
	using arrays_t = vuh::typelist<float, float>;      // value types of kernel array parameters
	using params_t = struct {                          // non-array parameters to kernel (push-constants), should mirror exactly the corresponding structure in the shader
		uint32_t size;                                  // array size
		float a;                                        // saxpy scaling parameter
	};

	auto kernel = vuh::Kernel<specs_t, params_t, arrays_t>("../shaders/saxpy.spv"); // define the kernel by linking interface and spir-v implementation
	kernel.on(device).batch(128/64)(specs_t{64}, {128, 0.1f}, d_y, d_x); // run once, wait for completion
	d_y.toHost(y);                                     // copy data back to host

	{ // same but split in steps and run 10 times
		auto program = kernel.on(device);               // instantiate kernel on the device
		program.batch(128/64)                           // set number of wrokgroups to run
		       .bind(specs_t{64})                       // set the specialization constants
		       .bind({128, 0.1f}, d_y, d_x);            // bind arrays and non-array parameters
		for(size_t i = 0; i < 10; ++i){
			program.run();
		}
	} // scoped to release gpu resources acquired by program.bind()

	return 0;
}
