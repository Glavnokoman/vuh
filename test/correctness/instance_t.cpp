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

TEST_CASE("test vuh::Instance"){
	SECTION("default instance"){
		REQUIRE_NOTHROW([]{
			auto instance = vuh::Instance();
		}());
	}
	SECTION("instance with some layers and extensions"){
		const auto available_layers = vuh::available_layers();
		const auto available_extensions = vuh::available_extensions();

		auto layer_names = std::vector<const char*>{};
		if(	!available_layers.empty()){
			layer_names = std::vector<const char*>{available_layers[0].layerName};
		}

		auto ext_names = std::vector<const char*>{};
		if(!available_extensions.empty()){
			ext_names = std::vector<const char*>{available_extensions[0].extensionName};
		}
		REQUIRE_NOTHROW([&]{
			auto instance = vuh::Instance(layer_names, ext_names);
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

//const char* __asan_default_options() { return "detect_leaks=0"; }
auto main( int argc, char* argv[])-> int {
	// global setup...
	int result = Catch::Session().run( argc, argv );
	// global clean-up...
	return result;
}
