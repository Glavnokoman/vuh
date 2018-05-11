#define CATCH_CONFIG_MAIN
#include <catch/catch.hpp>
#include "approx.hpp"

#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <vector>
#include <tuple>
#include <cstdint>

using test::approx;

TEST_CASE("saxpy_once", "[correctness]"){
	auto y = std::vector<float>(128, 1.0f);
	auto x = std::vector<float>(128, 2.0f);
	const auto a = 0.1f; // saxpy scaling constant

	auto out_ref = y;
	for(size_t i = 0; i < y.size(); ++i){
		out_ref[i] += a*x[i];
	}

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);            // just get the first compute-capable device

	auto d_y = vuh::Array<float>::fromHost(device, y); // allocate memory on device and copy data from host
	auto d_x = vuh::Array<float>::fromHost(device, x); // same for x

	using specs_t = std::tuple<uint32_t>;              // specialization constants, here it is the workgroup size
	using arrays_t = vuh::typelist<float, float>;      // value types of kernel array parameters
	using params_t = struct {                          // non-array parameters to kernel (push-constants), should mirror exactly the corresponding structure in the shader
		uint32_t size;                                  // array size
		float a;                                        // saxpy scaling parameter
	};

	auto kernel = vuh::Kernel<specs_t, params_t, arrays_t>("../shaders/saxpy.spv"); // define the kernel by linking interface and spir-v implementation
	kernel.on(device).grid(128/64).spec(specs_t{64})({128, a}, d_y, d_x); // run once, wait for completion
	d_y.toHost(y);                                     // copy data back to host

	REQUIRE(y == approx(out_ref).eps(1.e-5).verbose());
}

TEST_CASE("saxpy_repeated", "[correctness]"){
	auto y = std::vector<float>(128, 1.0f);
	auto x = std::vector<float>(128, 2.0f);
	const auto a = 0.1f; // saxpy scaling constant
	const auto n_repeat = 10;

	auto out_ref = y;
	for(size_t i = 0; i < y.size(); ++i){
		out_ref[i] += n_repeat*a*x[i];
	}

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);            // just get the first compute-capable device

	auto d_y = vuh::Array<float>::fromHost(device, y); // allocate memory on device and copy data from host
	auto d_x = vuh::Array<float>::fromHost(device, x); // same for x

	using specs_t = std::tuple<uint32_t>;              // specialization constants, here it is the workgroup size
	using arrays_t = vuh::typelist<float, float>;      // value types of kernel array parameters
	using params_t = struct {                          // non-array parameters to kernel (push-constants), should mirror exactly the corresponding structure in the shader
		uint32_t size;                                  // array size
		float a;                                        // saxpy scaling parameter
	};

	auto kernel = vuh::Kernel<specs_t, params_t, arrays_t>("../shaders/saxpy.spv"); // define the kernel by linking interface and spir-v implementation
	auto program = kernel.on(device);               // instantiate kernel on the device
	program.grid(128/64)                           // set number of wrokgroups to run
			 .spec(specs_t{64})                       // set the specialization constants
			 .bind({128, a}, d_y, d_x);	              // bind arrays and non-array parameters
	for(size_t i = 0; i < n_repeat; ++i){
		program.run();
	}
	d_y.toHost(y);                                     // copy data back to host

	REQUIRE(y == approx(out_ref).eps(1.e-5).verbose());
}
