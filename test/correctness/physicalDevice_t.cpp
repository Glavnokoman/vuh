//#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <vuh/physicalDevice.hpp>
#include <vuh/instance.hpp>

#include <iostream>

TEST_CASE("test physical devices", "[physicalDevice]"){
	auto instance = vuh::Instance();
	SECTION("physical devices"){
		for(auto d: instance.devices()){
			std::cout << d.properties().deviceName << "\n";
		}
	}
}

