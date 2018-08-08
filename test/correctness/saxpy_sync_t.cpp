#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "approx.hpp"

#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <vector>
#include <cstdint>

using test::approx;

TEST_CASE("saxpy_once_1D", "[program][correctness]"){
	auto y = std::vector<float>(128, 1.0f);
	auto x = std::vector<float>(128, 2.0f);
	const auto a = 0.1f; // saxpy scaling constant

	auto out_ref = y;
	for(size_t i = 0; i < y.size(); ++i){
		out_ref[i] += a*x[i];
	}

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);  // just get the first compute-capable device

	auto d_y = vuh::Array<float>(device, y); // allocate memory on device and copy data from host
	auto d_x = vuh::Array<float>(device, x); // same for x

	SECTION("nonempty specialization and push constants"){
		using Specs = vuh::typelist<uint32_t>;
		struct Params{uint32_t size; float a;};
		auto program = vuh::Program<Specs, Params>(device, "../shaders/saxpy.spv"); // define the kernel by linking interface and spir-v implementation
		program.grid(128/64).spec(64)({128, a}, d_y, d_x); // run once, wait for completion
		d_y.toHost(begin(y));                              // copy data back to host

		REQUIRE(y == approx(out_ref).eps(1.e-5).verbose());
	}
	SECTION("no specialization constants"){
		struct Params{uint32_t size; float a;};
		auto program = vuh::Program<vuh::typelist<>, Params>(device, "../shaders/saxpy_nospec.spv");
		program.grid(2)({128, a}, d_y, d_x);
		d_y.toHost(begin(y));

		REQUIRE(y == approx(out_ref).eps(1.e-5));
	}
	SECTION("no push constants"){
		using Specs = vuh::typelist<uint32_t>;
		auto program = vuh::Program<Specs>(device, "../shaders/saxpy_nopush.spv");
		program.grid(2).spec(64)(d_y, d_x);
		d_y.toHost(begin(y));

		REQUIRE(y == approx(out_ref).eps(1.e-5));
	}
	SECTION("no push or specialization constants"){
		auto program = vuh::Program<>(device, "../shaders/saxpy_noth.spv");
		program.grid(2)(d_y, d_x);
		d_y.toHost(begin(y));

		REQUIRE(y == approx(out_ref).eps(1.e-5));
	}
}

TEST_CASE("saxpy_repeated_1D", "[correctness]"){
	auto y = std::vector<float>(128, 1.0f);
	auto x = std::vector<float>(128, 2.0f);
	const auto a = 0.1f; // saxpy scaling constant
	const auto n_repeat = 10;

	auto out_ref = y;
	for(size_t i = 0; i < y.size(); ++i){
		out_ref[i] += n_repeat*a*x[i];
	}

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);

	auto d_y = vuh::Array<float>(device, y);
	auto d_x = vuh::Array<float>(device, x);

	SECTION("bind once run multiple"){
		using Specs = vuh::typelist<uint32_t>;
		struct Params{uint32_t size; float a;};
		auto program = vuh::Program<Specs, Params>(device, "../shaders/saxpy.spv");
		program.grid(128/64)                // set number of wrokgroups to run
		       .spec(64)                    // set the specialization constants
		       .bind({128, a}, d_y, d_x);   // bind arrays and non-array parameters
		for(size_t i = 0; i < n_repeat; ++i){
			program.run();
		}
		d_y.toHost(begin(y));

		REQUIRE(y == approx(out_ref).eps(1.e-5));
	}
	SECTION("multiple bind and run"){
		using Specs = vuh::typelist<uint32_t>;
		struct Params{uint32_t size; float a;};
		auto program = vuh::Program<Specs, Params>(device, "../shaders/saxpy.spv");
		program.grid(128/64).spec(64);
		for(size_t i = 0; i < n_repeat; ++i){
			program({128, a}, d_y, d_x);
		}
		d_y.toHost(begin(y));

		REQUIRE(y == approx(out_ref).eps(1.e-5));
	}
}
