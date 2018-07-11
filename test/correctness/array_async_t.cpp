#include <catch/catch.hpp>

#include <vuh/vuh.h>
#include <vuh/array.hpp>
#include <vuh/arr/copy_async.hpp>

#include <iostream>

using std::begin;
using std::end;

TEST_CASE("async copy device-local memory", "[array][correctness][async]"){
	constexpr auto arr_size = size_t(128);
	const auto host_data = std::vector<float>(arr_size, 3.14f);
//	const auto host_data_doubled = [&]{
//		auto r = host_data;
//		for(auto& x: r){ x *= 2.f; }
//		return r;
//	}();

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);

	SECTION("device-local memory"){
		SECTION("data transfer from host"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, arr_size);
			auto fence = vuh::copy_async(begin(host_data), end(host_data), begin(array));
			fence.wait();
			REQUIRE(array.toHost<std::vector<float>>() == host_data);
		}
		SECTION("data transfer to host"){
		}
		SECTION("async copy to host. scoped"){
		}
		SECTION("async copy from host. scoped"){
		}
		SECTION("async copy to host. iterators"){
		}
		SECTION("async copy from host. iterators"){
		}
	}
//	SECTION("device-local host-visible (unified) memory"){
//		try{
//			auto d1 = vuh::Array<float, vuh::mem::Unified>(device, arr_size);
//		} catch (...){
//			std::cerr << "skipping unified memory array tests..." "\n";
//			return;
//		}

//		SECTION("size constructor"){
//			auto array = vuh::Array<float, vuh::mem::Unified>(device, arr_size);
//			REQUIRE(array.size() == arr_size);
//			REQUIRE(array.size_bytes() == arr_size*sizeof(float));
//		}
//		SECTION("size constructor with value initialization"){
//			auto array = vuh::Array<float, vuh::mem::Unified>(device, arr_size, 3.14f);
//			REQUIRE(array.size() == arr_size);
//			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data);
//		}
//		SECTION("construct from range"){
//			auto array = vuh::Array<float, vuh::mem::Unified>(device, begin(host_data), end(host_data));
//			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data);
//		}
//		SECTION("random access with operator []"){
//			auto array = vuh::Array<float, vuh::mem::Unified>(device, arr_size, 3.14f);
//			REQUIRE(array[arr_size/2] == Approx(3.14f));
//			array[arr_size/2] = 2.71f;
//			REQUIRE(array[arr_size/2] == Approx(2.71f));
//		}
//		SECTION("begin() and end() iterators"){
//			auto array = vuh::Array<float, vuh::mem::Unified>(device, arr_size, 3.14f);
//			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data);
//			for(auto& x: array){
//				x *= 2.f;
//			}
//			REQUIRE(std::vector<float>(begin(array), end(array)) == host_data_doubled);
//		}
//	}
}
