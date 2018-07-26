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

	auto instance = vuh::Instance();
	auto device = instance.devices().at(0);

	SECTION("device-local memory to/from host"){
		SECTION("data transfer from host"){
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
		SECTION("async copy to host. 2 halves"){
			auto array = vuh::Array<float, vuh::mem::Device>(device, arr_size);
			{
				auto f1 = vuh::copy_async(begin(host_data), begin(host_data) + arr_size/2
				                          , device_begin(array));
				auto f2 = vuh::copy_async(begin(host_data) + arr_size/2, end(host_data)
				                          , device_begin(array) + arr_size/2);
			}
			REQUIRE(array.toHost<std::vector<float>>() == host_data);
		}
		SECTION("data transfer to host"){
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
	SECTION("device-local memory to/from host-visible"){

	}
//	SECTION("device-local host-visible (unified) memory"){
//		}
//	}
}
