#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "approx.hpp"

//#include <vuh/vuh.h>
#include <vuh/algorithm.hpp>
#include <vuh/buffer.hpp>
#include <vuh/kernel.hpp>
#include <vuh/queue.hpp>
#include <vuh/traits.hpp>

#include <vector>
#include <cstdint>

using test::approx;

namespace {
	constexpr auto buffer_size = 1024*64u;
	constexpr auto tile_size = buffer_size/2;
	constexpr auto workgroup_size = 64u;

	const auto x = std::vector<float>(buffer_size, 2.0f);
	const auto y = std::vector<float>(buffer_size, 1.0f);
	const auto a = 0.1f; // saxpy scaling constant

	auto instance = vuh::Instance();
	auto device = vuh::Device(instance);  // default (first available) device

	auto d_y = vuh::Buffer<float>(device, buffer_size); // allocate device buffers
	auto d_x = vuh::Buffer<float>(device, begin(x), end(x));
} // namespace

TEST_CASE("data transfer and computation interleaved. sync host side.", "[correctness][async]"){
	const auto ref = [&]{
		auto ret = y;
		for(size_t i = 0; i < ret.size(); ++i){ ret[i] += a*x[i];}
		return ret;
	}();

	auto out = std::vector<float>(y.size(), -1.f);

	SECTION("3-phase saxpy. default queues. verbose."){

		// phase 1. copy tiles to device
		auto fence_cpy = vuh::copy_async(begin(y), begin(y) + tile_size, d_y);
		auto fence_cpx = vuh::copy_async(begin(x), begin(x) + tile_size, d_x);

		// phase 2. process first tiles, copy over the second portion,
		fence_cpy.wait();
		fence_cpx.wait();

		struct {uint32_t size; float a;} p{tile_size, a};
		auto kernel = vuh::Kernel(device, "../shaders/saxpy.spv");
		auto fence_p = vuh::run_async(
		                  kernel.spec(workgroup_size).push(p).grid({tile_size/workgroup_size})
		                        .bind(d_y.span(0, tile_size), d_x.span(0, tile_size)));
		fence_cpy = vuh::copy_async(begin(y) + tile_size, end(y), d_y.span(tile_size, tile_size));
		fence_cpx = vuh::copy_async(begin(x) + tile_size, end(x), d_x.span(tile_size, tile_size));

		//	phase 3. copy back first result tile, run kernel on second tiles.
		fence_p.wait();
		auto fence_cpy_back1 = vuh::copy_async(d_y.span(0, tile_size), begin(out));
		fence_cpy.wait();
		fence_cpx.wait();
		fence_p = vuh::run_async(kernel.push(p).bind( d_y.span(tile_size, tile_size)
		                                            , d_x.span(tile_size, tile_size)));

		// wait for everything to complete
		fence_p.wait();
		auto fence_cpy_back2 = vuh::copy_async(d_y.span(tile_size, tile_size), begin(out) + tile_size);

		fence_cpy_back1.wait();
		fence_cpy_back2.wait();

		REQUIRE(y == approx(ref).eps(1.e-5).verbose());
	}
	SECTION("3-phase saxpy. default queues. scoped."){
		struct {uint32_t size; float a;} p{tile_size, a};
		auto kernel = vuh::Kernel(device, "../shaders/saxpy.spv");

		{ // phase 1. copy tiles to device
			auto f1 = vuh::copy_async(begin(y), begin(y) + tile_size, d_y);
			vuh::copy_async(begin(x), begin(x) + tile_size, d_x);
		}
		{ // phase 2. process first tiles, copy over the second portion,
			auto f_p = vuh::run_async(kernel.grid({tile_size/workgroup_size}).spec(workgroup_size)
			                                .push(p)
			                                .bind(d_y.span(0, tile_size), d_x.span(0, tile_size)));
			auto f_y = vuh::copy_async(begin(y) + tile_size, end(y), d_y.span(tile_size, tile_size));
			auto f_x = vuh::copy_async(begin(x) + tile_size, end(x), d_x.span(tile_size, tile_size));
		}
		{ // phase 3. copy back first result tile, run kernel on second tiles
			auto f_y1 = vuh::copy_async(d_y.span(0, tile_size), begin(out));

			vuh::run_async(kernel.bind(d_y.span(tile_size, tile_size), d_x.span(tile_size, tile_size)));
			auto f_y2 = vuh::copy_async(d_y.span(tile_size, tile_size), begin(out) + tile_size);
			f_y1.wait();
		}
		REQUIRE(y == approx(ref).eps(1.e-5).verbose());
	}

}
