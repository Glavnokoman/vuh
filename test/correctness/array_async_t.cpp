#include <catch2/catch.hpp>

#include <vuh/vuh.h>
#include <vuh/array.hpp>
#include <vuh/arr/copy_async.hpp>

#include <iostream>

using std::begin;
using std::end;

TEST_CASE("async copy device-local memory", "[array][correctness][async]"){
	constexpr auto arr_size = size_t(128);
	auto host_data = [](){
		auto r = std::vector<float>(arr_size, 3.14f);
		std::fill(begin(r) + arr_size/2, end(r), 6.28f);
		return r;
	}();

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);

	SECTION("in-device async copy from host-visible to device-local"){
		auto array_src = vuh::Array<float, vuh::mem::HostCoherent>(device, arr_size);
		std::copy(begin(host_data), end(host_data), begin(array_src));

		SECTION("explicit fence"){
			auto array_dst = vuh::Array<float, vuh::mem::Device>(device, arr_size);
			auto fence = vuh::copy_async(device_begin(array_src), device_end(array_src)
			                             , device_begin(array_dst));
			fence.wait();
			REQUIRE(array_dst.toHost<std::vector<float>>() == host_data);
		}
		SECTION("scoped"){
			auto array_dst = vuh::Array<float, vuh::mem::Device>(device, arr_size);
			{
				auto fence = vuh::copy_async(device_begin(array_src), device_end(array_src)
				                             , device_begin(array_dst));
			}
			REQUIRE(array_dst.toHost<std::vector<float>>() == host_data);
		}
		SECTION("copy 2 halves, scoped"){
			const auto chunk_size = arr_size/2;
			auto array_dst = vuh::Array<float, vuh::mem::Device>(device, arr_size);
			{
				auto f1 = vuh::copy_async(device_begin(array_src), device_begin(array_src) + chunk_size
				                          , device_begin(array_dst));
				auto f2 = vuh::copy_async(device_begin(array_src) + chunk_size, device_end(array_src)
				                          , device_begin(array_dst) + chunk_size);
			}
			REQUIRE(array_dst.toHost<std::vector<float>>() == host_data);
		}
	}
	SECTION("device-local memory to/from host"){
		SECTION("async copy from host. explicit wait()"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, arr_size);
			auto fence = vuh::copy_async(begin(host_data), end(host_data), device_begin(array));
			fence.wait();
			REQUIRE(array.toHost<std::vector<float>>() == host_data);
		}
		SECTION("async copy from host. scoped"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, arr_size);
			{
				auto fence = vuh::copy_async(begin(host_data), end(host_data), device_begin(array));
			}
			REQUIRE(array.toHost<std::vector<float>>() == host_data);
		}
		SECTION("async copy from host. 2 halves, scoped"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, arr_size);
			{
				auto f1 = vuh::copy_async(begin(host_data), begin(host_data) + arr_size/2
				                          , device_begin(array));
				auto f2 = vuh::copy_async(begin(host_data) + arr_size/2, end(host_data)
				                          , device_begin(array) + arr_size/2);
			}
			REQUIRE(array.toHost<std::vector<float>>() == host_data);
		}
		SECTION("async copy to host. explicit wait"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, host_data);
			auto host_data_tst = std::vector<float>(arr_size, 0.f);
			auto fence = vuh::copy_async(device_begin(array), device_end(array)
			                             , begin(host_data_tst));
			fence.wait();
			REQUIRE(host_data_tst == host_data);
		}
		SECTION("async copy to host. scoped"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, host_data);
			auto host_data_tst = std::vector<float>(arr_size, 0.f);
			{
				auto fence = vuh::copy_async(device_begin(array), device_end(array)
				                             , begin(host_data_tst));
			}
			REQUIRE(host_data_tst == host_data);
		}
		SECTION("async copy from host. 2 halves"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, host_data);
			auto host_data_tst = std::vector<float>(arr_size, 0.f);
			{
				auto f1 = vuh::copy_async(device_begin(array), device_begin(array) + arr_size/2
				                          , begin(host_data_tst));
				auto f2 = vuh::copy_async(device_begin(array) + arr_size/2, device_end(array)
				                          , begin(host_data_tst) + arr_size/2);
			}
			REQUIRE(host_data_tst == host_data);
		}
	}
}
