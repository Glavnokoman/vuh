#include <catch2/catch.hpp>

#include "helpers.h"

#include <vuh/vuh.h>
#include <vuh/array.hpp>
#include <vuh/arr/copy_async.hpp>

#include <iostream>
#include <random>

using std::begin;
using std::end;

namespace {
constexpr auto arr_size = size_t(1024);

const auto hy = test::random_vector<float>(arr_size, 0.f, 100.f);
const auto hx = test::random_vector<float>(arr_size, 0.f, 100.f);
const auto scale_mult = 2.0f; // saxpy mult
const auto out_ref = test::saxpy(hy, hx, scale_mult);

auto instance = vuh::Instance();
auto pdev = instance.phys_devices().at(0);
} // namespace

TEST_CASE("queues", "[correctness][async]"){
	auto device = pdev.computeDevice();
	auto d_in = vuh::Array<float>(device, arr_size);
	auto d_out = vuh::Array<float>(device, arr_size);

	using Specs = vuh::typelist<uint32_t>;
	struct Params{uint32_t size; float a;};
	auto program = vuh::Program<Specs, Params>(device, "../shaders/saxpy.spv");

	SECTION("mind free float"){
		//	auto cb = device.queues(1).copy(blabla).cb(); // compute barrier
		//	auto tb = cb.compute(blabla).tb();            // transfer barrier
		//	cb = tb.copy(albalb).cb();
		//	auto e1 = t1.copy(asdasd).event();

		//	auto q2 = device.queues(2);
		//	auto e2 = q2.on(e1).copy(asdasd).event()

		//	auto q3 = device.queues(3).event();
		//	auto e3 = q3.on(e1).compute(zxczxc);

		//	q1.on(e2, e3).compute(asdasd).cb().copy(albalb);
	}
	SECTION("mixed queues 1"){
		device.createQueues(MixedQueues(pdev.numMixedQueues()));

		const auto patch_size = arr_size/device.numMixedQueues();
		const auto block_size = 128;
		auto h_out = std::vector<float>(arr_size);
		auto off = size_t(0);
		for(auto& q: device.mixedQueues()){
			auto cb = q.copy(&hy[off], &hy[off+patch_size], vuh::array_view(d_in, off)).cb();
			auto tb = cb.run(program.grid(patch_size/block_size)
			                        .spec(block_size)
			                        .bind( {patch_size, scale_mult}
			                             , vuh::array_view(d_in, off, off + patch_size))
			                             , vuh::array_view(d_out, off, off + patch_size));
			tb.copy(d_out.view(off, patch_size), &h_out[off]);
			off += patch_size;
		}
		for(auto& q: device.mixedQueues()){ q.wait(); }
		REQUIRE(true == false);
	}
}
