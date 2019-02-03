#include <catch2/catch.hpp>

#include <vuh/vuh.h>
#include <vuh/array.hpp>

#include <iomanip>
#include <iostream>

using std::begin;
using std::end;

namespace {
	auto instance = vuh::Instance();
} // namespace

TEST_CASE("initialization & system info", "[correctness]"){
	SECTION("physical devices properties and queues"){
		for(const auto& pd: instance.physicalDevices()){
			std::cout << "physical device: " << pd.getProperties().deviceName << "\n";

			for(const auto& qf: pd.queueFamilies()){
				std::cout << "\tfamily " << qf.id() << "\n"
				          << "\tsupports compute: " << (qf.canCompute() ? "yes" : "no") << "\n"
				          << "\tsupports transfer: " << (qf.canTransfer() ? "yes" : "no") << "\n"
				          << "\thas queues: " << qf.queueCount << "\n"
				          << "\tflags: " << std::hex << unsigned(qf.queueFlags) << "\n";
			}
		}
	}
	SECTION("queues assignment"){
		auto pd = instance.physicalDevices().at(0);
		auto dev0 = pd.computeDevice(); // default, best allocation of 1 transport and 1 compute queue
		auto dev1 = pd.computeDevice(vuh::queueOptions::Default);    // same default
		auto dev2 = pd.computeDevice(vuh::queueOptions::AllQueues);  // claim all queues
		auto dev3 = pd.computeDevice(vuh::queueOptions::Streams{4}); // best possible allocation of queues to support 4 streams
		auto dev4 = pd.computeDevice(vuh::queueOptions::Specs
		                             { {0 /*family_id*/, 4 /*number queues*/, {/*priorities*/}}
		                             , {1, 2, {}}
		                             , {2, 1, {}} });
		for(size_t i = 0; i < dev4.queueCount(); ++i){
			auto q_i = dev4.queue(i);
		}
	}
}
