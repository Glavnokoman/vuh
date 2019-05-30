#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

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
constexpr auto arr_size = size_t(128);
const auto host_data = std::vector<float>(arr_size, 3.14f);
const auto host_data_doubled = [h = host_data]()mutable {
	for(auto& x: h){x *= 2.0f;}
	return h;
}();

auto instance = vuh::Instance();
auto device = vuh::Device(instance);
} // namespace

TEST_CASE("array with memory directly allocated from device", "[buffer]"){

	SECTION("device-only memory"){
		auto buf = vuh::Buffer<float, vuh::mem::DeviceOnly>(device, arr_size);
		REQUIRE(buf.size_bytes() == arr_size*sizeof(float));
	}
	SECTION("device-local memory"){
		SECTION("size constructor"){
			auto buf = vuh::Buffer<float, vuh::mem::Device>(device, arr_size);
			REQUIRE(buf.size() == arr_size);
			REQUIRE(buf.size_bytes() == arr_size*sizeof(float));

			auto buf_int = vuh::Buffer<int32_t, vuh::mem::Device>(device, arr_size);
			REQUIRE(buf_int.size() == arr_size);
			REQUIRE(buf_int.size_bytes() == arr_size*sizeof(int32_t));
		}
		SECTION("size, value constructor"){
			auto buf = vuh::Buffer<float, vuh::mem::Device>(device, arr_size, 3.14f);
			REQUIRE(vuh::to_host<std::vector<float>>(buf) == host_data);
		}
		SECTION("construct from host range"){
			auto buf = vuh::Buffer<float, vuh::mem::Device>(device, begin(host_data), end(host_data));
			REQUIRE(vuh::to_host<std::vector<float>>(buf) == host_data);
		}
		SECTION("data transfer from host to device"){
			auto buf = vuh::Buffer<float, vuh::mem::Device>(device, arr_size);
			vuh::copy(begin(host_data), end(host_data), buf);
			REQUIRE(vuh::to_host<std::vector<float>>(buf) == host_data);
			vuh::transform(begin(host_data), end(host_data), buf, [](auto x){return 2.f*x;});
			REQUIRE(vuh::to_host<std::vector<float>>(buf) == host_data_doubled);
		}
		SECTION("buffer transfer from device to host"){
			auto buf = vuh::Buffer<float, vuh::mem::Device>(device, begin(host_data), end(host_data));
			auto host_dst = std::vector<float>(arr_size, 0.f);
			vuh::copy(buf, begin(host_dst));
			REQUIRE(host_dst == host_data);
			vuh::transform(buf, begin(host_dst), [](auto x){return 2.f*x;});
			REQUIRE(host_dst == host_data_doubled);
		}
		SECTION("transfer buffer span to host"){
			auto buf = vuh::Buffer<float, vuh::mem::Device>(device, begin(host_data), end(host_data));
			auto host_dst = std::vector<float>(arr_size, 0.f);
			vuh::copy(buf.span(0, arr_size/2), begin(host_dst));
			REQUIRE_FALSE(host_dst == host_data);
			vuh::copy(buf.span(arr_size/2, arr_size/2), begin(host_dst) + arr_size/2);
			REQUIRE(host_dst == host_data);

			vuh::transform(buf.span(0, arr_size/2), begin(host_dst), [](auto x){return 2.f*x;});
			REQUIRE_FALSE(host_dst == host_data_doubled);
			vuh::transform(buf.span(arr_size/2, arr_size/2), begin(host_dst) + arr_size/2
			               , [](auto x){return 2.f*x;});
			REQUIRE(host_dst == host_data_doubled);
		}
	}
	SECTION("device-local host-visible (unified) memory"){
		try{
			auto d1 = vuh::Buffer<float, vuh::mem::Unified>(device, arr_size);
		} catch (...){
			Catch::cerr() << "skipping unified memory array tests..." "\n";
			return;
		}
		SECTION("size constructor"){
			auto buf = vuh::Buffer<float, vuh::mem::Unified>(device, arr_size);
			REQUIRE(buf.size() == arr_size);
			REQUIRE(buf.size_bytes() == arr_size*sizeof(float));
		}
		SECTION("size constructor with value initialization"){
			auto buf = vuh::Buffer<float, vuh::mem::Unified>(device, arr_size, 3.14f);
			REQUIRE(buf.size() == arr_size);
			REQUIRE(std::vector<float>(begin(buf), end(buf)) == host_data);
		}
		SECTION("construct from range"){
			auto buf = vuh::Buffer<float, vuh::mem::Unified>(device, std::begin(host_data), std::end(host_data));
			REQUIRE(std::vector<float>(begin(buf), end(buf)) == host_data);
		}
		SECTION("random access with operator []"){
			auto buf = vuh::Buffer<float, vuh::mem::Unified>(device, arr_size, 3.14f);
			REQUIRE(buf[arr_size/2] == Approx(3.14f));
			buf[arr_size/2] = 2.71f;
			REQUIRE(buf[arr_size/2] == Approx(2.71f));
		}
		SECTION("begin() and end() iterators"){
			auto buf = vuh::Buffer<float, vuh::mem::Unified>(device, arr_size, 3.14f);
			REQUIRE(std::vector<float>(begin(buf), end(buf)) == host_data);
			for(auto& x: buf){ x *= 2.f; }
			REQUIRE(std::vector<float>(begin(buf), end(buf)) == host_data_doubled);
		}
	}
	SECTION("host pinned memory"){
		SECTION("size constructor"){
			auto buf = vuh::Buffer<float, vuh::mem::Host>(device, arr_size);
			REQUIRE(buf.size() == arr_size);
			REQUIRE(buf.size_bytes() == arr_size*sizeof(float));
		}
		SECTION("size constructor with value initialization"){
			auto buf = vuh::Buffer<float, vuh::mem::Host>(device, arr_size, 3.14f);
			REQUIRE(buf.size() == arr_size);
			REQUIRE(std::vector<float>(begin(buf), end(buf)) == host_data);
		}
		SECTION("construct froim range"){
			auto buf = vuh::Buffer<float, vuh::mem::Host>(device, begin(host_data), end(host_data));
			REQUIRE(std::vector<float>(begin(buf), end(buf)) == host_data);
		}
		SECTION("random access with operator []"){
			auto buf = vuh::Buffer<float, vuh::mem::Host>(device, arr_size, 3.14f);
			REQUIRE(buf[arr_size/2] == Approx(3.14f));
			buf[arr_size/2] = 2.71f;
			REQUIRE(buf[arr_size/2] == Approx(2.71f));
		}
		SECTION("begin() and end() iterators"){
			auto buf = vuh::Buffer<float, vuh::mem::Host>(device, arr_size, 3.14f);
			REQUIRE(std::vector<float>(begin(buf), end(buf)) == host_data);
			for(auto& x: buf){ x *= 2.f; }
			REQUIRE(std::vector<float>(begin(buf), end(buf)) == host_data_doubled);
		}
	}
	SECTION("void memory allocator should throw"){
		REQUIRE_THROWS(([&](){
			auto d_buf = vuh::Buffer<float, vuh::AllocatorDevice<void>>(device, arr_size);
		}()));
	}
}
