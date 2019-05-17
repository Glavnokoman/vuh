#include "device.hpp"
#include "error.hpp"
#include "instance.hpp"
#include "physicalDevice.hpp"
#include "queue.hpp"

#include <cassert>
#include <tuple>

using std::begin; using std::end;
//#define ALL(c) begin(c), end(c)
#define ARR_VIEW(x) uint32_t(x.size()), x.data()

namespace vuh {
namespace {
///
auto make_device( const PhysicalDevice& phys_dev
                , const std::vector<QueueSpec>& specs
                , const std::vector<const char*>& extensions
                )-> VkDevice
{
	auto queue_info = std::vector<VkDeviceQueueCreateInfo>{};
	for(const auto& spec: specs){
		queue_info.push_back({VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
	                        , nullptr
	                        , VkDeviceQueueCreateFlags{}
	                        , spec.family_id
		                     , spec.queue_count
		                     , spec.priorities.data()
		                     });
	}
	const auto features = phys_dev.features();
	auto dev = VkDevice{};
	const auto createInfo = VkDeviceCreateInfo{
	     VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
	   , nullptr // pNext
	   , VkDeviceCreateFlags{}
	   , ARR_VIEW(queue_info)
	   , 0, nullptr // enabled layers. deprecated
	   , ARR_VIEW(extensions)
	   , &features
	};
	const auto err = vkCreateDevice(phys_dev, &createInfo, nullptr, &dev);
	VUH_CHECK_RET(err, dev);
	return dev;
}

///
auto select_queue_family( const std::vector<VkQueueFamilyProperties>& families
                        , VkQueueFlags target_flags
                        )-> std::size_t
{
	auto ret = std::size_t(-1);
	auto min_flags = VkFlags(VK_QUEUE_FLAG_BITS_MAX_ENUM);

	for(size_t i = 0; i < families.size(); ++i){
		const auto flags_i = families[i].queueFlags;
		if(0 < families[i].queueCount
		   && ((target_flags & flags_i) == target_flags) // all target flags are present
		   && flags_i < min_flags
		){
			ret = i;
			min_flags = flags_i;
		}
	}
	return ret;
}

///
struct DefaultQueues { Queue* compute; Queue* transfer; };
///
auto select_default_queue( Instance& instance
                         , const std::vector<VkQueueFamilyProperties>& families ///< all physical device families
                         , std::vector<Queue>& queues ///< logical device queue handles. will be std::span<> one day
                         , const std::vector<QueueSpec>& queue_specs
                         )-> DefaultQueues
{
	auto selected_families = std::vector<VkQueueFamilyProperties>{};
	for(const auto& spec: queue_specs) if(spec.queue_count != 0u){
		selected_families.push_back(families[spec.family_id]);
	}

	auto get_queue_offset = [&](std::size_t family_offset){
		auto compute_queue_id = 0u;
		auto family_count = 0u;
		for(const auto& spec: queue_specs) if(spec.queue_count != 0u){
			if(family_count++ == family_offset){ break; }
			compute_queue_id += spec.queue_count;
		}
		return compute_queue_id;
	};

	const auto offset_compute_family = select_queue_family(selected_families, VK_QUEUE_COMPUTE_BIT);
	if(offset_compute_family == std::size_t(-1)){
		instance.log("vuh device construction"
		             , "no queue family specified with compute capability bit set"
		             , VK_DEBUG_REPORT_WARNING_BIT_EXT);
	}

	const auto offset_transfer_family = select_queue_family(selected_families, VK_QUEUE_TRANSFER_BIT);
	if(offset_transfer_family == std::size_t(-1)){
		instance.log("vuh device construction"
		             , "no queue family specified with transfer capability bit set"
		             , VK_DEBUG_REPORT_WARNING_BIT_EXT);
	}
	return { &queues[get_queue_offset(offset_compute_family)]
	       , &queues[get_queue_offset(offset_transfer_family)] };
}

/// Translate queue specification preset to actual queue specification for a given physical device
auto to_specs( const PhysicalDevice& phys_device
             , QueueSpec::Preset preset
             )-> std::vector<QueueSpec>
{
	const auto families = phys_device.queueFamilies();
	switch(preset){
		case QueueSpec::Default: {
			auto offset_allmighty = select_queue_family( families
			                                           , VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
			if(offset_allmighty != std::size_t(-1)){
				return {{uint32_t(offset_allmighty), 1, {1.0f}}};
			}
			auto offset_compute = select_queue_family(families, VK_QUEUE_COMPUTE_BIT);
			assert(offset_compute != std::size_t(-1));
			auto offset_transfer = select_queue_family(families, VK_QUEUE_TRANSFER_BIT);
			assert(offset_transfer != std::size_t(-1));
			return { {uint32_t(offset_compute), 1, {1.f}}
			       , {uint32_t(offset_transfer), 1, {1.f}}};
		}
		case QueueSpec::All:{
			auto ret = std::vector<QueueSpec>{};
			for(uint32_t i = 0; i <  families.size(); ++i) {
				if(families[i].queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT)) {
					ret.push_back({i, families[i].queueCount, std::vector(families[i].queueCount, 1.f)});
				}
			}
			return ret;
		}
	}

}
} // namespace

/// Wrapping constructor. This is also the most basic constructor being called by the all the others.
Device::Device( VkDevice device
              , Instance& instance
              , const PhysicalDevice& phys_device
              , const std::vector<QueueSpec>& queue_specs ///< each family_id can have only one corresponding entry
              )
   : Device::Base(device), _instance(instance), _physical{phys_device}
{
	const auto queue_families = phys_device.queueFamilies(); // all queue families on a device
	_command_pools = {};
	for(const auto& spec: queue_specs){ // first sweep - create command pools per queue family
		const auto& queue_family = queue_families[spec.family_id];
		const auto pool_flags = ((queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0
		                         ? VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		                         : VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		auto create_info = VkCommandPoolCreateInfo{
		     VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
		   , nullptr
		   , pool_flags
		   , spec.family_id
		};
		auto pool = VkCommandPool{};
		VUH_CHECK(vkCreateCommandPool(device, &create_info, nullptr, &pool));
		_command_pools.push_back(pool);
	}

	for(size_t i = 0; i < queue_specs.size(); ++i){ // sweep 2 - create queue handles
		const auto& spec = queue_specs[i];
		auto& pool = _command_pools[i];
		for(std::uint32_t i = 0; i < spec.queue_count; ++i){
			auto queue = VkQueue{};
			vkGetDeviceQueue(device, spec.family_id, i, &queue);
			_queues.push_back(Queue(queue, *this, pool, spec.family_id));
		}
	}

	const auto pipecache_info = VkPipelineCacheCreateInfo {
	                            VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, nullptr
	                            , {}, 0, nullptr };
	VUH_CHECK(vkCreatePipelineCache(device, &pipecache_info, nullptr, &_pipecache));

	// point to default compute and transport queues
	auto defaults = select_default_queue(instance, queue_families, _queues, queue_specs);
	_default_compute = defaults.compute;
	_default_transfer = defaults.transfer;
}

///
Device::Device(Instance& instance
              , const PhysicalDevice& physical_device
              , const std::vector<QueueSpec>& specs
              , const std::vector<const char*>& extensions
              )
   : Device(make_device(physical_device, specs, extensions)
           , instance, physical_device, specs)
{}

///
Device::Device(Instance& instance
               , const PhysicalDevice& phys_device
               , QueueSpec::Preset preset
               , const std::vector<const char*>& extensions
               )
	: Device(instance, phys_device, to_specs(phys_device, preset), extensions)
{}

///
Device::Device(Instance& instance, const std::vector<const char*>& extensions)
   : Device(instance, instance.devices().at(0), QueueSpec::Default, extensions)
{}

///
Device::~Device() noexcept {
	for(const auto& pool: _command_pools){
		vkDestroyCommandPool(*this, pool, nullptr);
	}
	vkDestroyPipelineCache(*this, _pipecache, nullptr);
}

} // namespace vuh
