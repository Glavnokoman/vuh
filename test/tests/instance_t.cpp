#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <vuh/instance.hpp>
#include <vuh/utils.hpp>

namespace {
//
struct StubLogger {
	static unsigned call_count;
	static std::string last_message;
	static auto log( VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t
	               , const char* layer_prefix
	               , const char* message
	               , void*       /*pUserData*/)-> VkBool32
	{
		call_count += 1;
		last_message = std::string(layer_prefix) + ": " + message;
		return VK_FALSE;
	}
}; // struct StubLogger
unsigned StubLogger::call_count = 0;
std::string StubLogger::last_message = {};
} // namespace

TEST_CASE("test vuh::Instance", "[Instance]"){
	SECTION("default instance"){
		REQUIRE_NOTHROW([]{
			auto instance = vuh::Instance();
		}());
	}
	SECTION("list extensions and layers, create instance with some"){
		const auto available_layers = vuh::available_layers();
		const auto available_extensions = vuh::available_extensions();

		const char* test_layer = "VK_LAYER_LUNARG_core_validation";
		if(std::find_if(begin(available_layers), end(available_layers)
		                , [test_layer](VkLayerProperties lpt){
		                      return std::strcmp(lpt.layerName, test_layer) == 0;})
		   == end(available_layers))
		{
			std::cerr << "test layer " << test_layer << " is not available. skipping test" << std::endl;
			return;
		}

		const char* test_ext = "VK_KHR_surface";
		if(std::find_if(begin(available_extensions), end(available_extensions)
		                , [test_ext](VkExtensionProperties ex){
		                    return std::strcmp(ex.extensionName, test_ext) == 0;})
		   == end(available_extensions))
		{
			std::cerr << "test extension " << test_ext << "is not available. skipping test" << std::endl;
			return;
		}

		REQUIRE_NOTHROW([&]{
			auto instance = vuh::Instance({test_layer}, {test_ext});
		}());
	}
	SECTION("instance with missing layers/extensions"){
		REQUIRE_THROWS([]{
			auto instance = vuh::Instance({"huy"}, {});
		}());
		REQUIRE_THROWS([]{
			auto instance = vuh::Instance({}, {"huy"});
		}());
	}
	SECTION("check logger"){
		auto instance = vuh::Instance();
		instance.logger_attach(StubLogger::log);
		instance.log("qwe", "asd");
		REQUIRE(StubLogger::last_message.find("qwe") != std::string::npos);
		REQUIRE(StubLogger::last_message.find("asd") != std::string::npos);
	}
}
TEST_CASE("benchmark instance creation", "[instance][benchmark]"){
	BENCHMARK("vuh instance"){
		auto instance = vuh::Instance();
	}
	BENCHMARK("vulkan instance"){
		auto instance = VkInstance{};
		auto create_info = VkInstanceCreateInfo{};
		(void)vkCreateInstance(&create_info, nullptr, &instance);
		vkDestroyInstance(instance, nullptr);
	}
}

//const char* __asan_default_options() { return "detect_leaks=0"; }
auto main( int argc, char* argv[])-> int {
	// global setup...
	int result = Catch::Session().run( argc, argv );
	// global clean-up...
	return result;
}
