#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "approx.hpp"

//#include <vuh/vuh.hpp>
#include <vuh/algorithm.hpp>
#include <vuh/buffer.hpp>
#include <vuh/kernel.hpp>
#include <vuh/traits.hpp>

#include <vector>
#include <cstdint>

using test::approx;

TEST_CASE("saxpy 1D, run once", "[kernel]"){
	auto y = std::vector<float>(128, 1.0f);
	auto x = std::vector<float>(128, 2.0f);
	const auto a = 0.1f; // saxpy scaling constant

	auto ref = y;
	for(size_t i = 0; i < y.size(); ++i){ ref[i] += a*x[i]; }

	auto instance = vuh::Instance();
	auto device = vuh::Device(instance);  // default (first available) device

	auto d_y = vuh::Buffer<float>(device, begin(y), end(y)); // allocate device buffer and copy data
	auto d_x = vuh::Buffer<float>(device, begin(x), end(x));

	SECTION("nonempty specialization and push constants"){
//		auto program = vuh::Program(device, "../shaders/saxpy.spv");
//		auto kernel = program.kernel("main");
		auto kernel = vuh::Kernel(device, "../shaders/saxpy.spv");
		struct {uint32_t b; float a;} p{128u, a}; // struct Push
		vuh::run(kernel.spec(64u).push(p).bind(d_y, d_x).grid({128/64}));
		vuh::copy(d_y, begin(y));

		REQUIRE(y == approx(ref).eps(1.e-5).verbose());
	}
//	SECTION("no specialization constants"){
//		auto kernel = vuh::Kernel(device, "../shaders/saxpy_nospec.spv");
//		struct {uint32_t b; float a;} p{128u, a}; // struct Push
//		vuh::run(kernel.push(p).bind(d_y, d_x).grid({128/64}));
//		vuh::copy(d_y, begin(y));

//		REQUIRE(y == approx(ref).eps(1.e-5));
//	}
//	SECTION("no push constants"){
//		auto kernel = vuh::Kernel(device, "../shaders/saxpy_nopush.spv");
//		vuh::run(kernel.spec(64u).bind(d_y, d_x).grid({128/64}));
//		vuh::copy(d_y, begin(y));

//		REQUIRE(y == approx(ref).eps(1.e-5));
//	}
//	SECTION("no push or specialization constants"){
//		auto kernel = vuh::Kernel(device, "../shaders/saxpy_noth.spv");
//		vuh::run(kernel.bind(d_y, d_x).grid({128/64}));
//		vuh::copy(d_y, begin(y));

//		REQUIRE(y == approx(ref).eps(1.e-5));
//	}
}

TEST_CASE("saxpy_repeated_1D", "[.kernel]"){
	auto y = std::vector<float>(128, 1.0f);
	auto x = std::vector<float>(128, 2.0f);
	const auto a = 0.1f; // saxpy scaling constant
	const auto n_repeat = 10;

	auto out_ref = y;
	for(size_t i = 0; i < y.size(); ++i){
		out_ref[i] += n_repeat*a*x[i];
	}

	auto instance = vuh::Instance();
	auto device = vuh::Device(instance);  // default (first available) device

	auto d_y = vuh::Buffer<float>(device, begin(y), end(y)); // allocate device buffer and copy data
	auto d_x = vuh::Buffer<float>(device, begin(x), end(x));

	static_assert(vuh::traits::is_device_buffer_v<vuh::Buffer<float>> == true);
	SECTION("bind once run multiple"){
		auto kernel = vuh::Kernel(device, "../shaders/saxpy.spv");
		struct {uint32_t b; float a;} p{128u, a}; // struct Push
		kernel.spec(64u).push(p).bind(d_y, d_x).grid({128/64});
		for(size_t i = 0; i < n_repeat; ++i){
			vuh::run(kernel);
		}
		vuh::copy(d_y, begin(y));

		REQUIRE(y == approx(out_ref).eps(1.e-5));
	}
	SECTION("multiple bind and run"){
		auto kernel = vuh::Kernel(device, "../shaders/saxpy.spv");
		kernel.spec(64u);
		struct {uint32_t b; float a;} p{128u, a}; // struct Push
		for(size_t i = 0; i < n_repeat; ++i){
			vuh::run(kernel.push(p).bind(d_y, d_x).grid({128/64}));
		}
		vuh::copy(d_y, begin(y));

		REQUIRE(y == approx(out_ref).eps(1.e-5));
	}
}
