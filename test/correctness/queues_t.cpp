#include <catch2/catch.hpp>

#include "helpers.h"
#include "approx.hpp"

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
auto pdev = instance.physicalDevices().at(0);
} // namespace

TEST_CASE("queues", "[correctness][async]"){
	auto device = pdev.computeDevice();
	auto d_y = vuh::Array<float>(device, arr_size);
	auto d_x = vuh::Array<float>(device, arr_size);

	using Specs = vuh::typelist<uint32_t>;
	struct Params{uint32_t size; float a;};
	auto program = vuh::Program<Specs, Params>(device, "../shaders/saxpy.spv");

	SECTION("queues"){

	}
	SECTION("streams"){
		auto streams = device.makeStreams();
		const auto n_streams = streams.size();

		const auto patch_size = uint32_t(arr_size/n_streams);
		const auto block_size = 128;
		auto h_out = vuh::Array<float, vuh::mem::HostCached>(device, arr_size);
		SECTION("fine-grained stream config"){
			auto transfer_queue_ids = std::vector<uint32_t>{};
			auto compute_queue_ids = std::vector<uint32_t>{};
			for(size_t i = 0; i < device.queues().size(); ++i){
				const auto& q_i = device.queues()[i];
				if(q_i.canCompute()){
					compute_queue_ids.push_back(i);
				}
				if(q_i.canTransfer()){
					compute_queue_ids.push_back(i);
				}
			}
			auto queue_config = std::vector<vuh::StreamSpec>{};
			for(size_t i = 0; i < n_streams; ++i){
				queue_config.push_back({ compute_queue_ids[i % compute_queue_ids.size()]
				                       , transfer_queue_ids[i % transfer_queue_ids.size()]});
			}
			auto fine_streams = device.makeStreams(queue_config);
			REQUIRE(fine_streams.size() == n_streams);
		}
		SECTION("streams 1"){
			auto off = size_t(0);
			for(auto& q: streams){
				auto cb = q.copy(&hy[off], &hy[off+patch_size], vuh::array_view(d_y, off, off+patch_size))
				           .copy(&hx[off], &hx[off+patch_size], vuh::array_view(d_x, off, off+patch_size))
				           .cb();
				auto tb = cb.run(program.grid(patch_size/block_size).spec(block_size)
				                        .bind( {patch_size, scale_mult}
				                             , vuh::array_view(d_y, off, off + patch_size)
				                             , vuh::array_view(d_x, off, off + patch_size))
				                 ).tb();
				tb.copy(vuh::array_view(d_y, off, patch_size), vuh::array_view(h_out, off, off+patch_size));
				off += patch_size;
			}
			for(auto& q: streams){ q.wait(); } // wait till queues are empty
			
			REQUIRE(h_out == test::approx(out_ref).eps(1e-5));
		}
		SECTION("streams 2"){
			auto off = size_t(0);
			for(auto& q: streams){
				q.copy(&hy[off], &hy[off+patch_size], vuh::array_view(d_y, off, off+patch_size))
				 .copy(&hx[off], &hx[off+patch_size], vuh::array_view(d_x, off, off+patch_size))
				 .cb()
				 .run(program.grid(patch_size/block_size).spec(block_size)
				             .push(patch_size, scale_mult)
				             .bind( vuh::array_view(d_y, off, off + patch_size)
				                  , vuh::array_view(d_x, off, off + patch_size)))
				 .tb()
				 .copy(vuh::array_view(d_y, off, patch_size), vuh::array_view(h_out, off, off+patch_size));
				off += patch_size;
			}
			for(auto& q: streams){ q.wait(); } // wait till queues are empty
			
			REQUIRE(h_out == test::approx(out_ref).eps(1e-5));
		}
		SECTION("streams 3"){
			auto cbs = std::vector<vuh::BarrierCompute>{};
			auto off = size_t(0);
			for(auto& q: streams){
				q.copy(&hy[off], &hy[off+patch_size], vuh::array_view(d_y, off, off+patch_size));
				q.copy(&hx[off], &hx[off+patch_size], vuh::array_view(d_x, off, off+patch_size));
				cbs.push_back(q.cb());
				off += patch_size;
			}
			
			auto tbs = std::vector<vuh::BarrierTransfer>{};
			off = 0;
			program.grid(patch_size/block_size).spec(block_size).push(patch_size, scale_mult);
			for(auto& cb: cbs){
				program.bind( vuh::array_view(d_y, off, off + patch_size)
				            , vuh::array_view(d_x, off, off + patch_size));
				tbs.push_back(cb.run(program).tb());
				off += patch_size;
			}
			
			auto hbs = std::vector<vuh::BarrierHost>{};
			off = 0;
			for(auto& tb: tbs){
				hbs.push_back(tb.copy(vuh::array_view(d_y, off, patch_size)
				                      , vuh::array_view(h_out, off)).hb());
				off += patch_size;
			}
			for(auto& hb: hbs){ hb.wait(); }
			
			REQUIRE(h_out == test::approx(out_ref).eps(1e-5));
		}
		SECTION("inter-queue sync"){
			program.grid(patch_size/block_size).spec(block_size).push(patch_size, scale_mult);
			
			auto e1 = queues[0].copy(&hy[0], &hy[arr_size], d_y).qb(); // vuh::BarrierQueue
			auto e2 = queues[1].copy(&hx[0], &hx[arr_size], d_x).qb();
			auto e3 = queues[2].on(e1, e2).run(
			             program.bind( vuh::array_view(d_y, 0, arr_size/2)
			                         , vuh::array_view(d_x, 0, arr_size/2))
			             ).qb();
			auto e4 = queues[3].on(e1, e2).run(
			             program.bind( vuh::array_view(d_y, arr_size/2, arr_size)
			                         , vuh::array_view(d_x, arr_size/2, arr_size))
			             ).qb();
			auto hb = queues[0].on(e3).copy( vuh::array_view(d_y, 0, arr_size/2) 
			                               , vuh::array_view(h_out, 0, arr_size/2)).hb();
			queues[1].on(e4).copy( vuh::array_view(d_y, arr_size/2, arr_size) 
			                     , vuh::array_view(h_out, arr_size/2, arr_size)).hb();
			hb.wait();
			
			REQUIRE(h_out == test::approx(out_ref).eps(1e-5));
		}
	}
}
