#include <catch2/catch.hpp>
#include "approx.hpp"

#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <vector>
#include <cstdint>

using test::approx;

TEST_CASE("data transfer and computation interleaved. sync host side.", "[correctness][async]"){
	constexpr auto arr_size = 128;
	const auto tile_size = arr_size/2;

	auto y = std::vector<float>(arr_size, 1.0f);
	auto x = std::vector<float>(arr_size, 2.0f);
	std::fill(begin(x) + tile_size, end(x), 4.0);
	const auto a = 0.1f; // saxpy scaling constant

	auto out_ref = y;
	for(size_t i = 0; i < y.size(); ++i){
		out_ref[i] += a*x[i];
	}

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);  // just get the first compute-capable device

	auto d_y = vuh::Array<float>(device, arr_size); // allocate memory on device
	auto d_x = vuh::Array<float>(device, arr_size);

	const auto grid_x = 32;

//	auto cap = device.hasSeparateQueues(); // check if compute queues family is same as transfer queues
	SECTION("3-phase saxpy. default queues. verbose."){

		// phase 1. copy tiles to device
		auto fence_cpy = vuh::copy_async(begin(y), begin(y) + tile_size, device_begin(d_y));
		auto fence_cpx = vuh::copy_async(begin(x), begin(x) + tile_size, device_begin(d_x));

		// phase 2. process first tiles, copy over the second portion,
		fence_cpy.wait();
		fence_cpx.wait();

		using Specs = vuh::typelist<uint32_t>;
		struct Params{uint32_t size; float a;};
		auto program = vuh::Program<Specs, Params>(device, "../shaders/saxpy.spv"); // define the kernel by linking interface and spir-v implementation
		auto fence_p = program.grid(tile_size/grid_x).spec(grid_x)
		                      .run_async({tile_size, a}, vuh::array_view(d_y, 0, tile_size)
		                                               , vuh::array_view(d_x, 0, tile_size));
		fence_cpy = vuh::copy_async(begin(y) + tile_size, end(y), device_begin(d_y) + tile_size);
		fence_cpx = vuh::copy_async(begin(x) + tile_size, end(x), device_begin(d_x) + tile_size);

		//	phase 3. copy back first result tile, run kernel on second tiles.
		fence_p.wait();
		auto fence_cpy_back1 = vuh::copy_async(device_begin(d_y), device_begin(d_y) + tile_size
		                                       , begin(y));
		fence_cpy.wait();
		fence_cpx.wait();
		fence_p = program.run_async({tile_size, a}, vuh::array_view(d_y, tile_size, arr_size)
		                                          , vuh::array_view(d_x, tile_size, arr_size));

		// wait for everything to complete
		fence_p.wait();
		auto fence_cpy_back2 = vuh::copy_async(device_begin(d_y) + tile_size, device_end(d_y)
		                                       , begin(y) + tile_size);

		fence_cpy_back1.wait();
		fence_cpy_back2.wait();

		REQUIRE(y == approx(out_ref).eps(1.e-5).verbose());
	}
	SECTION("3-phase saxpy. default queues. scoped."){
		using Specs = vuh::typelist<uint32_t>;
		struct Params{uint32_t size; float a;};
		auto program = vuh::Program<Specs, Params>(device, "../shaders/saxpy.spv");

		{ // phase 1. copy tiles to device
			auto f1 = vuh::copy_async(begin(y), begin(y) + tile_size, device_begin(d_y));
			vuh::copy_async(begin(x), begin(x) + tile_size, device_begin(d_x));
		}

		{ // phase 2. process first tiles, copy over the second portion,
			auto f_p = program.grid(tile_size/grid_x).spec(grid_x)
			                  .run_async({tile_size, a}, vuh::array_view(d_y, 0, tile_size)
			                                           , vuh::array_view(d_x, 0, tile_size));
			auto f_y = vuh::copy_async(begin(y) + tile_size, end(y), device_begin(d_y) + tile_size);
			auto f_x = vuh::copy_async(begin(x) + tile_size, end(x), device_begin(d_x) + tile_size);
		}

		{ // phase 3. copy back first result tile, run kernel on second tiles
			auto f_y1 = vuh::copy_async(device_begin(d_y), device_begin(d_y) + tile_size, begin(y));

			program.run_async({tile_size, a}, vuh::array_view(d_y, tile_size, arr_size)
			                                , vuh::array_view(d_x, tile_size, arr_size));
			auto f_y2 = vuh::copy_async(device_begin(d_y) + tile_size, device_end(d_y), begin(y) + tile_size);
			f_y1.wait();
		}

		REQUIRE(y == approx(out_ref).eps(1.e-5).verbose());
	}

}
