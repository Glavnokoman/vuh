#include <vuh/array.hpp>
#include <vuh/vuh.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <fstream>
#include <vector>

namespace spd = spdlog;

using std::begin;

static auto logger = spd::basic_logger_mt("logger", "vuh.log");

/// this will print messages using spdlog logger
auto report_callback( VkDebugReportFlagsEXT flags
                    , VkDebugReportObjectTypeEXT /*objtype*/
                    , uint64_t    /*object*/
                    , size_t      /*location*/
                    , int32_t     /*message_code*/
                    , const char* pLayerPrefix
                    , const char* pMessage
                    , void*       /*pUserData*/
                    )-> uint32_t 
{
	switch(flags){
		case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
			logger->info("{}:{}", pLayerPrefix, pMessage);
			break;
		case VK_DEBUG_REPORT_WARNING_BIT_EXT:
		case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
			logger->warn("{}:{}", pLayerPrefix, pMessage);
			break;
		case VK_DEBUG_REPORT_ERROR_BIT_EXT:
			logger->error("{}:{}", pLayerPrefix, pMessage);
			break;
		case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
			logger->debug("{}:{}", pLayerPrefix, pMessage);
			break;
		default:
			logger->info("{}:{}", pLayerPrefix, pMessage);
	}
	return 0;
}


auto main()-> int {
	auto instance = vuh::Instance({}, {}, {nullptr, 0, nullptr, 0, VK_API_VERSION_1_0}, report_callback);
	auto device = instance.devices().at(0); // just get the first compute-capable device

	try{
		auto a = vuh::Array<float>(device, size_t(-1));
	} catch (vk::OutOfDeviceMemoryError& err) {
		instance.report("explicit message log", err.what(), VK_DEBUG_REPORT_ERROR_BIT_EXT);
	}

   return 0;
}
