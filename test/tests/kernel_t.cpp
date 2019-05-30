#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "approx.hpp"

//#include <vuh/vuh.hpp>
#include <vuh/algorithm.hpp>
#include <vuh/buffer.hpp>
#include <vuh/kernel.hpp>
#include <vuh/traits.hpp>

#include <vector>
#include <cstdint>

using test::approx;

namespace {

constexpr auto buffer_size = 128u;
constexpr auto workgroup_size = 64u;

const auto x = std::vector<float>(buffer_size, 2.0f);
const auto y = std::vector<float>(buffer_size, 1.0f);
const auto a = 0.1f; // saxpy scaling constant

auto instance = vuh::Instance();
auto device = vuh::Device(instance);  // default (first available) device

auto d_y = vuh::Buffer<float>(device, buffer_size); // allocate device buffers
auto d_x = vuh::Buffer<float>(device, begin(x), end(x));

} // namespace

TEST_CASE("test kernel execution", "[kernel][sync]"){
	auto out = std::vector<float>(buffer_size, -1.f);
	vuh::copy(begin(y), end(y), d_y); // reinitialize d_y

	SECTION("synchronous saxpy 1D, run once"){
		const auto ref = [&]{
			auto ret = y;
			for(size_t i = 0; i < ret.size(); ++i){ ret[i] += a*x[i]; }
			return ret;
		}();

		SECTION("nonempty specialization and push constants"){
			auto kernel = vuh::Kernel(device, "../shaders/saxpy.spv");
			struct {uint32_t b; float a;} p{buffer_size, a};
			vuh::run(kernel.spec(workgroup_size).push(p).bind(d_y, d_x)
			               .grid({buffer_size/workgroup_size})
			);

			vuh::copy(d_y, begin(out));
			REQUIRE(out == approx(ref).eps(1.e-5).verbose());
		}
		SECTION("no specialization constants"){
			auto kernel = vuh::Kernel(device, "../shaders/saxpy_nospec.spv");
			struct {uint32_t b; float a;} p{buffer_size, a};
			vuh::run(kernel.push(p).bind(d_y, d_x).grid({buffer_size/workgroup_size}));
			vuh::copy(d_y, begin(out));
			REQUIRE(out == approx(ref).eps(1.e-5));
		}
		SECTION("no specialization constants"){
			auto kernel = vuh::Kernel(device, "../shaders/saxpy_nopush.spv");
			vuh::run(kernel.spec(workgroup_size).bind(d_y, d_x).grid({buffer_size/workgroup_size}));
			vuh::copy(d_y, begin(out));
			REQUIRE(out == approx(ref).eps(1.e-5));
		}
		SECTION("no push or specialization constants"){
			auto kernel = vuh::Kernel(device, "../shaders/saxpy_noth.spv");
			vuh::run(kernel.bind(d_y, d_x).grid({buffer_size/workgroup_size}));
			vuh::copy(d_y, begin(out));
			REQUIRE(out == approx(ref).eps(1.e-5));
		}
	}
	SECTION("synchronous saxpy, multiple kernel rerun"){
		const auto n_repeat = 10;
		const auto ref = [&]{
			auto ret = y;
			for(size_t i = 0; i < ret.size(); ++i){ ret[i] += a*n_repeat*x[i]; }
			return ret;
		}();
		SECTION("bind once run multiple"){
			auto kernel = vuh::Kernel(device, "../shaders/saxpy.spv");
			struct {uint32_t b; float a;} p{buffer_size, a}; // struct Push
			kernel.spec(workgroup_size).push(p).bind(d_y, d_x).grid({buffer_size/workgroup_size});
			for(size_t i = 0; i < n_repeat; ++i){
				vuh::run(kernel);
			}
			vuh::copy(d_y, begin(out));
			REQUIRE(out == approx(ref).eps(1.e-5));
		}
		SECTION("multiple bind and run"){
			auto kernel = vuh::Kernel(device, "../shaders/saxpy.spv");
			kernel.spec(workgroup_size);
			struct {uint32_t b; float a;} p{buffer_size, a}; // struct Push
			for(size_t i = 0; i < n_repeat; ++i){
				vuh::run(kernel.push(p).bind(d_y, d_x).grid({buffer_size/workgroup_size}));
			}
			vuh::copy(d_y, begin(out));
			REQUIRE(out == approx(ref).eps(1.e-5));
		}
	}
}
