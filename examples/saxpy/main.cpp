#include <vuh/array.hpp>
#include <vuh/vuh.h>
#include <vector>

auto main()-> int {
   auto y = std::vector<float>(128, 1.0f);
	auto x = std::vector<float>(128, 2.0f);
	const auto a = 0.1f; // saxpy scaling constant

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);  // just get the first compute-capable device

	auto d_y = vuh::Array<float>(device, y); // allocate memory on device and copy data from host
	auto d_x = vuh::Array<float>(device, x); // same for x

	using Specs = vuh::typelist<uint32_t>;
	struct Params{uint32_t size; float a;};
	auto program = vuh::Program<Specs, Params>(device, "saxpy.spv"); // define the kernel by linking interface and spir-v implementation
	program.grid(128/64).spec(64)({128, a}, d_y, d_x); // run once, wait for completion
	d_y.toHost(begin(y));                              // copy data back to host

   return 0;
}
