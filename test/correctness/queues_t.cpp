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
auto pdev = instance.physDevices().at(0);
} // namespace

TEST_CASE("queues", "[correctness][async]"){
	auto device = pdev.computeDevice();
	auto d_y = vuh::Array<float>(device, arr_size);
	auto d_x = vuh::Array<float>(device, arr_size);

	using Specs = vuh::typelist<uint32_t>;
	struct Params{uint32_t size; float a;};
	auto program = vuh::Program<Specs, Params>(device, "../shaders/saxpy.spv");

	SECTION("mixed queues"){
		constexpr auto n_queues = 4;
		auto queues = device.mixedQueues(n_queues); // should work even if there are not enough physical queues
		REQUIRE(queues.size() == n_queues);

		const auto patch_size = arr_size/n_queues;
		const auto block_size = 128;
		auto h_out = vuh::Array<float, vuh::mem::HostCached>(device, arr_size);
		SECTION("fine-grained mixed queue config"){
			auto transfer_queue_ids = std::vector<size_t>{};
			auto compute_queue_ids = std::vector<size_t>{};
			for(size_t i = 0; i < device.numQueues(); ++i){
				if(device.queue(i).canCompute()){
					compute_queue_ids.push_back(i);
				}
				if(device.queue(i).canTransfer()){
					compute_queue_ids.push_back(i);
				}
			}
			auto queue_config = std::vector<vuh::MixedQueueSpec>{};
			for(size_t i = 0; i < n_queues; ++i){
				queue_config.push_back({ compute_queue_ids[i % compute_queue_ids.size()]
				                       , transfer_queue_ids[i % transfer_queue_ids.size()]});
			}
			auto fine_queues = device.mixedQueues(queue_config);
			REQUIRE(fine_queues.size() == n_queues);
		}
		SECTION("mixed queues 1"){
			auto off = size_t(0);
			for(auto& q: queues){
				auto cb = q.copy(&hy[off], &hy[off+patch_size], vuh::array_view(d_y, off))
				           .copy(&hx[off], &hx[off+patch_size], vuh::array_view(d_x, off))
				           .cb();
				auto tb = cb.run(program.grid(patch_size/block_size).spec(block_size)
				                        .bind( {patch_size, scale_mult}
				                             , vuh::array_view(d_y, off, off + patch_size)
				                             , vuh::array_view(d_x, off, off + patch_size))
				                 ).tb();
				tb.copy(vuh::array_view(d_y, off, patch_size), vuh::array_view(h_out, off));
				off += patch_size;
			}
			for(auto& q: queues){ q.wait(); } // wait till queues are empty
			
			REQUIRE(h_out == test::approx(out_ref).eps(1e-5));
		}
		SECTION("mixed queues 2"){
			auto off = size_t(0);
			for(auto& q: queues){
				q.copy(&hy[off], &hy[off+patch_size], vuh::array_view(d_y, off))
				 .copy(&hx[off], &hx[off+patch_size], vuh::array_view(d_x, off))
				 .cb()
				 .run(program.grid(patch_size/block_size).spec(block_size)
				             .push(patch_size, scale_mult)
				             .bind( vuh::array_view(d_y, off, off + patch_size)
				                  , vuh::array_view(d_x, off, off + patch_size)))
				 .tb()
				 .copy(vuh::array_view(d_y, off, patch_size), vuh::array_view(h_out, off));
				off += patch_size;
			}
			for(auto& q: queues){ q.wait(); } // wait till queues are empty
			
			REQUIRE(h_out == test::approx(out_ref).eps(1e-5));
		}
		SECTION("mixed queues 3"){
			auto cbs = std::vector<vuh::BarrierCompute>{};
			auto off = size_t(0);
			for(auto& q: queues){
				q.copy(&hy[off], &hy[off+patch_size], vuh::array_view(d_y, off));
				q.copy(&hx[off], &hx[off+patch_size], vuh::array_view(d_x, off));
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
			
			auto e1 = queues[0].copy(&hy[0], &hy[arr_size], d_y).bq(); // vuh::BarrierQueue
			auto e2 = queues[1].copy(&hx[0], &hx[arr_size], d_x).bq();
			auto e3 = queues[2].on(e1, e2).run(
			             program.bind( vuh::array_view(d_y, 0, arr_size/2)
			                         , vuh::array_view(d_x, 0, arr_size/2))
			             ).bq();
			auto e4 = queues[3].on(e1, e2).run(
			             program.bind( vuh::array_view(d_y, arr_size/2, arr_size)
			                         , vuh::array_view(d_x, arr_size/2, arr_size))
			             ).bq();
			auto hb = queues[0].on(e3).copy( vuh::array_view(d_y, 0, arr_size/2) 
			                               , vuh::array_view(h_out, 0)).hb();
			queues[1].on(e4).copy( vuh::array_view(d_y, arr_size/2, arr_size) 
			                     , vuh::array_view(h_out, arr_size/2)).hb();
			hb.wait();
			
			REQUIRE(h_out == test::approx(out_ref).eps(1e-5));
		}
	}
}
