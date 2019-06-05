#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "approx.hpp"

//#include <vuh/vuh.h>
#include <vuh/algorithm.hpp>
#include <vuh/buffer.hpp>
#include <vuh/device.hpp>
#include <vuh/instance.hpp>

#include <algorithm>
#include <iostream>

using std::begin;
using std::end;

namespace {
constexpr auto arr_size = size_t(1024*256);
const auto host_data = [](){
	auto r = std::vector<float>(arr_size, 3.14f);
	std::fill(begin(r) + arr_size/2, end(r), 6.28f);
	return r;
}();

auto instance = vuh::Instance();
auto device = vuh::Device(instance);
} // namespace

TEST_CASE("async copy device-local memory", "[array][correctness][async]"){
	auto src = vuh::Buffer<float, vuh::mem::HostCoherent>(device, begin(host_data), end(host_data));
	auto dst = vuh::Buffer<float, vuh::mem::Device>(device, arr_size, -1.f);

	SECTION("in-device async copy from host-visible to device-local"){
		SECTION("explicit fence"){
			auto fence = vuh::copy_async(src, dst);
			fence.wait();
			REQUIRE(vuh::to_host<std::vector<float>>(dst) == host_data);
		}
		SECTION("scoped"){
			{
				auto fence = vuh::copy_async(src, dst);
			}
			REQUIRE(vuh::to_host<std::vector<float>>(dst) == host_data);
		}
		SECTION("copy 2 halves, scoped"){
			const auto chunk_size = arr_size/2;
			{ // @fixme: host-allocated buffer has no associated span
//				auto f1 = vuh::copy_async(src.span(0, chunk_size), dst);
//				auto f2 = vuh::copy_async( src.span(chunk_size, chunk_size)
//				                         , dst.span(chunk_size, chunk_size));
			}
//			REQUIRE(vuh::to_host<std::vector<float>>(dst) == host_data);
		}
	}
	SECTION("device-local memory from host"){
		SECTION("async copy from host. explicit wait()"){
			auto fence = vuh::copy_async(begin(host_data), end(host_data), dst);
			fence.wait();
			REQUIRE(vuh::to_host<std::vector<float>>(dst) == host_data);
		}
		SECTION("async copy from host. scoped"){
			{
				auto fence = vuh::copy_async(begin(host_data), end(host_data), dst);
			}
			REQUIRE(vuh::to_host<std::vector<float>>(dst) == host_data);
		}
        SECTION("async copy from host. 2 halves, scoped"){ // do not try it on nvidia+linux
			{
				auto f1 = vuh::copy_async( begin(host_data), begin(host_data) + arr_size/2
				                         , dst.span(0, arr_size/2));
//				f1.wait(); // debug
				auto f2 = vuh::copy_async( begin(host_data) + arr_size/2, end(host_data)
				                         , dst.span(arr_size/2, arr_size/2));
			}
			REQUIRE(vuh::to_host<std::vector<float>>(dst) == test::approx(host_data).verbose());
		}
	}
}
