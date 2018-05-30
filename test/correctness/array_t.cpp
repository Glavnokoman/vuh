#include <catch/catch.hpp>

#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <iostream>

using std::begin;
using std::end;

TEST_CASE("array_alloc_basic", "[array]"){
	constexpr auto arr_size = size_t(128);
	const auto h0 = std::vector<float>(arr_size, 3.14f);

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);

	SECTION("device-only memory"){
		auto d0 = vuh::Array<float, vuh::mem::DeviceOnly>(device, arr_size);
		REQUIRE(d0.size_bytes() == arr_size*sizeof(float));
	}
	SECTION("device memory"){
	   auto d1 = vuh::Array<float, vuh::mem::Device>(device, arr_size);
		REQUIRE(d1.size() == arr_size);

		d1.fromHost(begin(h0), end(h0));
		auto h1 = std::vector<float>(arr_size, 0.f);
		d1.toHost(begin(h1));
		REQUIRE(h0 == h1);

	   auto d4 = vuh::Array<float, vuh::mem::Device>(device, h0);
		REQUIRE(d4.size() == h0.size());

		auto h4_tst = std::vector<float>(arr_size, 0.f);
		d4.toHost(begin(h4_tst), [](auto x){return 2.f*x;});
		auto h4_ref = h0;
		for(auto& x: h4_ref){ x *= 2.f; }
		REQUIRE(h4_tst == h4_ref);

	   auto d6 = vuh::Array<float, vuh::mem::Device>(device, begin(h0), end(h0));
		REQUIRE(d6.size() == h0.size());

		auto h6_tst = std::vector<float>(arr_size, 0.f);
		d6.toHost(begin(h6_tst), arr_size/2, [](auto x){return 3.f*x;});
		d6.toHost(begin(h6_tst) + arr_size/2, arr_size/2, [](auto x){return 4.f*x;});
		auto h6_ref = h0;
		for(size_t i = 0; i < arr_size; ++i){
			h6_ref[i] *= (i < arr_size/2 ? 3.f : 4.f);
		}
		REQUIRE(h6_tst == h6_ref);

		auto d3 = vuh::Array<float, vuh::mem::Device>(device, arr_size,
	                                                 [&](size_t i){return h0[i];});
		REQUIRE(d3.size() == arr_size);

		auto h3_tst = d3.toHost<std::vector<float>>();
		REQUIRE(h3_tst == h0);

		// auto d7 = vuh::Array<float, vuh::pool::Device>(pool, arr_size);
	}
	SECTION("unified memory"){
		try{
			auto d1 = vuh::Array<float, vuh::mem::Unified>(device, arr_size);
		} catch (...){
			std::cerr << "unified memory allocation failed, but that might be OK..." "\n";
			return;
		}

		auto d2 = vuh::Array<float, vuh::mem::Unified>(device, arr_size, 2.71f);
		auto d6 = vuh::Array<float, vuh::mem::Unified>(device, begin(h0), end(h0));

	   // random-access iterable
		auto h1 = std::vector<float>(arr_size, 0.f);
	   std::copy(begin(d2), end(d2), begin(h1));
	   d6[42] = 42.f;
	}
	SECTION("host mappable memory"){
		auto d1 = vuh::Array<float, vuh::mem::Host>(device, arr_size);
	   auto d2 = vuh::Array<float, vuh::mem::Host>(device, arr_size, 2.71f);
	   auto d6 = vuh::Array<float, vuh::mem::Host>(device, begin(h0), end(h0));

	   // random access iterable
		auto h1 = std::vector<float>(arr_size, 0.f);
	   std::copy(begin(d2), end(d2), begin(h1));
	   d2[42] = 42.f;
	}
}
