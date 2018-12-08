#include <catch2/catch.hpp>

#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <iostream>

using std::begin;
using std::end;

namespace {
	auto instance = vuh::Instance();
} // namespace

TEST_CASE("initialization & system info", "[correctness]"){

	SECTION("physical devices properties and queues"){
		for(const auto& pd: instance.physDevices()){
			auto props = pd.getProperties(); // forwards vk::PhysicalDevice

			std::cout << "physical device: " << pd.name() << "\n"
			          << "\t compute enabled queues: " << pd.numComputeQueues() << "\n"
			          << "\t transfer enabled queues: " << pd.numTransferQueues() << "\n"
			          << "\t compute-only queues: " << pd.numComputeOnlyQueues() << "\n"
			          << "\t transfer-only queues: " << pd.numTransferOnlyQueues() << "\n"
			          << "\t multi-purpose queues: " << pd.numMixedQueues() << "\n";

			auto qfms = pd.queueFamilies();
			for(const auto& qf: qfms){
				std::cout << "family " << qf.id()
				          << "supports compute: " << (qf.isCompute() ? "yes" : "no")
				          << "supports transfer: " << (qf.isTransfer() ? "yes" : "no")
				          << "has queues: " << qf.numQueues()
				          << "flags: " << qf.flags() << "\n";
			}
		}
	}
	SECTION("create default compute device"){
		SECTION("default compute device"){
			auto dev = instance.physDevices().at(0).computeDevice(); // only default queues
		}
		SECTION("create compute device with mixed queues"){
			auto phys_dev = instance.physDevices().at(0);
			auto dev = phys_dev.computeDevice(MixedQueues(phys_dev.numMixedQueues()));
		}
		SECTION("create queues on default device"){
			auto phys_dev = instance.physDevices().at(0);
			auto dev = phys_dev.computeDevice(); // only default queues
			dev.resource(ComputeQueues(phys_dev.numComputeOnlyQueues())
			             , TransferQueues(phys_dev.numTransferOnlyQueues)
			             , MixedQueues(phys_dev.numMixedQueues()));
			REQUIRE(dev.numComputeOnlyQueues() == phys_dev.numComputeOnlyQueues());
			REQUIRE(dev.numTransferOnlyQueues() == phys_dev.numTransferOnlyQueues());
			REQUIRE(dev.numMixedQueues() == phys_dev.numMixedQueues());
		}
		SECTION("fine-grained queues creation"){
			auto phys_dev = instance.physDevices().at(0);
			auto dev = phys_dev.computeDevice();
			auto qfms = phys_dev.queueFamilies();
			constexpr auto id_mixed = 0;
			constexpr auto id_transfer = 1;
			constexpr auto id_compute = 2;
			if(qfms[id_compute].isCompute() && qfms[id_transfer].isTransfer()
			   && qfms[id_mixed].isCompute()){
				dev.resource(ComputeQueues({id_mixed, 1}, {id_compute, 1})
				             , TransferQueues({id_transfer, 1}));
			}

			using QueueAllocInfo = struct{uint16_t family_id; u_int16_t n_queues;};
			auto computeQueues = std::vector<vuh::QueueAllocInfo>{};
			auto mixedQueues = std::vector<vuh::QueueAllocInfo>{};
			for(const auto qf: qfms){
				if(qf.isCompute() && qf.isTranfer()){
					mixedQueues.push_back({qf.id(), std::min(2, qf.numQueues())});
				} else if(qf.isCompute()){
					computeQueues.push_back({qf.id(), std::min(2, qf.numQueues())});
				}
			}
			dev.resource(ComputeQueues(ALL(computeQueues)), mixedQueues(ALL(mixedQueues)));
		}
		SECTION("create combined queues"){
			// mixed-functionality logical queues combining compute-only and transfer-only queues
		}
	}

}
