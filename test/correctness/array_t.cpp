#include <catch2/catch.hpp>

#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <iostream>

using std::begin;
using std::end;

TEST_CASE("array with memory directly allocated from device", "[array][correctness]"){
	constexpr auto arr_size = size_t(128);
	const auto host_data = std::vector<float>(arr_size, 3.14f);
	const auto host_data_doubled = [&]{
		auto r = host_data;
		for(auto& x: r){ x *= 2.f; }
		return r;
	}();

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);

	SECTION("device-only memory"){
		auto array = vuh::Array<float, vuh::mem::DeviceOnly>(device, arr_size);
		REQUIRE(array.size_bytes() == arr_size*sizeof(float));
	}
	SECTION("device-local memory"){
		SECTION("size constructor"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, arr_size);
			REQUIRE(array.size() == arr_size);
			REQUIRE(array.size_bytes() == arr_size*sizeof(float));

			auto array_int = vuh::Array<int32_t, vuh::mem::Device>(device, arr_size);
			REQUIRE(array.size() == arr_size);
			REQUIRE(array.size_bytes() == arr_size*sizeof(int32_t));
		}
		SECTION("construct from range"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, begin(host_data), end(host_data));
			REQUIRE(array.toHost<std::vector<float>>() == host_data);
		}
		SECTION("contruct from iterable"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, host_data);
			REQUIRE(array.toHost<std::vector<float>>() == host_data);
		}
		SECTION("size + index based lambda constructor"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, arr_size
			                                                 , [&](size_t i){return host_data[i];});
			REQUIRE(array.toHost<std::vector<float>>() == host_data);
		}
		SECTION("data transfer from host range"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, arr_size);
			array.fromHost(begin(host_data), end(host_data));
			REQUIRE(array.toHost<std::vector<float>>() == host_data);
		}
		SECTION("transfer whole array to host with transform"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, host_data);
			auto host_dst = std::vector<float>(arr_size, 0.f);
			array.toHost(begin(host_dst), [](auto x){ return 2.f*x;});
			REQUIRE(host_dst == host_data_doubled);
		}
		SECTION("transfer n values to host with transform"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, host_data);
			auto host_dst = std::vector<float>(arr_size, 0.f);
			array.toHost(begin(host_dst), arr_size/2, [](auto x){ return 2.f*x;});
			REQUIRE_FALSE(host_dst == host_data_doubled);
			array.toHost(begin(host_dst), arr_size, [](auto x){ return 2.f*x;});
			REQUIRE(host_dst == host_data_doubled);
		}
		// this one is deliberately same as construct from iterable
		SECTION("transfer whole array to newly created host std::vector"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, host_data);
			REQUIRE(array.toHost<std::vector<float>>() == host_data);
		}
		// auto d7 = vuh::Array<float, vuh::pool::Device>(pool, arr_size);
	}
	SECTION("device-local host-visible (unified) memory"){
		try{
			auto d1 = vuh::Array<float, vuh::mem::Unified>(device, arr_size);
		} catch (...){
			std::cerr << "skipping unified memory array tests..." "\n";
			return;
		}

		SECTION("size constructor"){
			auto array = vuh::Array<float, vuh::mem::Unified>(device, arr_size);
			REQUIRE(array.size() == arr_size);
			REQUIRE(array.size_bytes() == arr_size*sizeof(float));
		}
		SECTION("size constructor with value initialization"){
			auto array = vuh::Array<float, vuh::mem::Unified>(device, arr_size, 3.14f);
			REQUIRE(array.size() == arr_size);
			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data);
		}
		SECTION("construct from range"){
			auto array = vuh::Array<float, vuh::mem::Unified>(device, begin(host_data), end(host_data));
			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data);
		}
		SECTION("random access with operator []"){
			auto array = vuh::Array<float, vuh::mem::Unified>(device, arr_size, 3.14f);
			REQUIRE(array[arr_size/2] == Approx(3.14f));
			array[arr_size/2] = 2.71f;
			REQUIRE(array[arr_size/2] == Approx(2.71f));
		}
		SECTION("begin() and end() iterators"){
			auto array = vuh::Array<float, vuh::mem::Unified>(device, arr_size, 3.14f);
			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data);
			for(auto& x: array){
				x *= 2.f;
			}
			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data_doubled);
		}
	}
	SECTION("host pinned memory"){
		SECTION("size constructor"){
			auto array = vuh::Array<float, vuh::mem::Host>(device, arr_size);
			REQUIRE(array.size() == arr_size);
			REQUIRE(array.size_bytes() == arr_size*sizeof(float));
		}
		SECTION("size constructor with value initialization"){
			auto array = vuh::Array<float, vuh::mem::Host>(device, arr_size, 3.14f);
			REQUIRE(array.size() == arr_size);
			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data);
		}
		SECTION("construct from range"){
			auto array = vuh::Array<float, vuh::mem::Host>(device, begin(host_data), end(host_data));
			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data);
		}
		SECTION("random access with operator []"){
			auto array = vuh::Array<float, vuh::mem::Host>(device, arr_size, 3.14f);
			REQUIRE(array[arr_size/2] == Approx(3.14f));
			array[arr_size/2] = 2.71f;
			REQUIRE(array[arr_size/2] == Approx(2.71f));
		}
		SECTION("begin() and end() iterators"){
			auto array = vuh::Array<float, vuh::mem::Host>(device, arr_size, 3.14f);
			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data);
			for(auto& x: array){
				x *= 2.f;
			}
			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data_doubled);
		}
	}
	SECTION("void memory allocator should throw"){
		REQUIRE_THROWS(([&](){
			auto d_array = vuh::Array<float, vuh::arr::AllocDevice<void>>(device, arr_size);
		}()));
	}
}
