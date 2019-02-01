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
			auto props = pd.getProperties(); // forwards vk::PhysicalDevice

			std::cout << "physical device: " << pd.getProperties().deviceName << "\n";

			for(const auto& qf: pd.queueFamilies()){
				std::cout << "family " << qf.id()
				          << "supports compute: " << (qf.canCompute() ? "yes" : "no")
				          << "supports transfer: " << (qf.canTransfer() ? "yes" : "no")
				          << "has queues: " << qf.queueCount
				          << "flags: " << std::hex << unsigned(qf.queueFlags) << "\n";
			}
		}
	}
	SECTION("queues assignment"){
		auto pd = instance.physicalDevices().at(0);
		auto dev0 = pd.computeDevice(); // default, best allocation of 1 transport and 1 compute queue
		auto dev1 = pd.computeDevice(vuh::devOpts::Default);    // same default
		auto dev2 = pd.computeDevice(vuh::devOpts::AllQueues);  // claim all queues
		auto dev3 = pd.computeDevice(vuh::devOpts::Streams{4}); // best possible allocation of queues to support 4 streams
		auto dev4 = pd.computeDevice(vuh::devOpts::QueueSpecs
		                             { {0 /*family_id*/, 4 /*number queues*/, {/*priorities*/}}
		                             , {1, 2, {}}
		                             , {2, 1, {}} });
		for(size_t i = 0; i < dev4.queueCount(); ++i){
			auto q_i = dev4.queue(i);
		}
	}
}
