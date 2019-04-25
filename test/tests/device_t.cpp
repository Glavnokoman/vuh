//#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <vuh/device.hpp>
#include <vuh/instance.hpp>
#include <vuh/physicalDevice.hpp>

TEST_CASE("test logical devices", "[device]"){
	auto instance = vuh::Instance();
	auto pd = instance.devices().at(0);
	SECTION("create default device"){
		auto device = vuh::Device(instance);
	}
	SECTION("create default device on a physical"){
		auto device = vuh::Device(instance, pd, vuh::QueueSpec::Default);
	}
	SECTION("device with all queues from a physical"){
		auto device2 = vuh::Device(instance, pd, vuh::QueueSpec::All);
	}
}
