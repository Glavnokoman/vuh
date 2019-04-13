#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include <vuh/instance.hpp>
#include <vuh/utils.hpp>

TEST_CASE("test vuh::Instance", "[]"){
	SECTION("default instance"){
		REQUIRE_NOTHROW([]{
			auto instance = vuh::Instance();
		}());
//		REQUIRE(true);
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

//		std::transform(begin(available_layers), end(available_layers), std::back_inserter(layer_names)
//		               , [](const auto& p){return p.layerName;});
//		std::transform(begin(available_extensions), end(available_extensions), std::back_inserter(ext_names)
//		               , [](const auto& p){return p.extensionName;});

		REQUIRE_NOTHROW([&]{
			auto instance = vuh::Instance(layer_names, ext_names);
		}());
	}
	SECTION("instance with missing layers/extensions"){
		REQUIRE_THROWS([]{
			auto instance = vuh::Instance({"huy"}, {});
		}());
	}
}

//const char* __asan_default_options() { return "detect_leaks=0"; }

auto main( int argc, char* argv[])-> int {
	// global setup...

	int result = Catch::Session().run( argc, argv );

	// global clean-up...

	return result;
}
