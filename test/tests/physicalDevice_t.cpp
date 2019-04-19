//#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <vuh/physicalDevice.hpp>
#include <vuh/instance.hpp>

#include <iostream>

TEST_CASE("test physical devices", "[physicalDevice]"){
	auto instance = vuh::Instance();
	SECTION("enumerate devices"){
		for(auto d: instance.devices()){
			std::cout << d.properties().deviceName << "\n";
		}
	}
	SECTION("enumerate queues"){
		auto total_queues = uint32_t(0);
		for(auto d: instance.devices()){
			for(auto q: d.queueFamilies()){
				total_queues += q.queueCount;
			}
		}
	}
}

