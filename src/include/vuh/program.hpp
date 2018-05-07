#pragma once

#include "array.hpp"
#include "device.h"

#include <vulkan/vulkan.hpp>

#include <array>
#include <cstddef>
#include <tuple>
#include <utility>

namespace {
	namespace detail {
		template<class T> struct DictTypeToDsc;
	
		template<> struct DictTypeToDsc<float>{ 
			static constexpr vk::DescriptorType value = vk::DescriptorType::eStorageBuffer;
		};
		
		template<> struct DictTypeToDsc<uint32_t>{ 
			static constexpr vk::DescriptorType value = vk::DescriptorType::eStorageBuffer; 
		};
		
		/// Not currently used, just a wild idea on how to support uniforms (and dynamic uniforms),
		/// needs check if that works. If not - descriptor types have to be passed as a parameter 
		/// to kernel constructor.
		template<> struct DictTypeToDsc<const float>{ 
			static constexpr vk::DescriptorType value = vk::DescriptorType::eUniformBuffer;
		};
	 
		// Compile-time offset of tuple element.
		// Probably UB as tuples are non-POD types (no easy way out till C++ gets some compile-time reflection).
		// Should never be called outside constexpr context.
		template<size_t Idx, class T>
		constexpr auto tuple_element_offset()-> std::size_t {
			constexpr T dum{};
			return size_t(reinterpret_cast<const char*>(&std::get<Idx>(dum))
		                 - reinterpret_cast<const char*>(&dum));
		}
		
		//
		template<class T, size_t... I>
		constexpr auto spec2entries(const T& /*specs*/, std::index_sequence<I...>){
			return std::array<vk::SpecializationMapEntry, sizeof...(I)>{{
			                       { uint32_t(I)
			                       , uint32_t(tuple_element_offset<I, T>())
			                       , uint32_t(sizeof(typename std::tuple_element<I, T>::type))
			                       }...
			}};
		}
	} // namespace trais

	/// doc me
	template<template<class...> class L, class... Ts>
	auto typesToDscTypes() {
		return std::array<vk::DescriptorType, sizeof...(Ts)>{detail::DictTypeToDsc<Ts>::value...};
	}
	
	/// doc me
	template<size_t N>
	auto dscTypesToLayout(const std::array<vk::DescriptorType, N>& dsc_types) {
		auto r = std::array<vk::DescriptorSetLayoutBinding, N>{};
		for(size_t i = 0; i < N; ++i){ // this can be done compile-time, but hardly worth it.
			r[i] = {uint32_t(i), dsc_types[i], 1, vk::ShaderStageFlagBits::eCompute};
		}
		return r;
	}
	
	/// @return specialization map array
	template<template<class...> class T, class... Ts>
	auto specs2mapentries(const T<Ts...>& specs
	                      )-> std::array<vk::SpecializationMapEntry, sizeof...(Ts)> 
	{
		return detail::spec2entries(specs, std::make_index_sequence<sizeof...(Ts)>{});
	}

	/// doc me
	template<class T, size_t... I>
	auto dscinfos2writesets(vk::DescriptorSet dscset, const T& infos
	                        , std::index_sequence<I...>)
	{
		auto r = std::array<vk::WriteDescriptorSet, sizeof...(I)>{{
			{dscset, uint32_t(I), 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &infos[I]}...
		}};
		return r;
	}
} // namespace

namespace vuh {
	template<class... Ts>	 struct typelist{};

	/// Runnable program. Allows to bind the actual parameters to the interface and execute 
	/// kernel on a Vulkan device.
	template<class S, class P, class A> class Program;
	
	/// specialization to unpack array types parameters
	template< class Specs  ///< tuple of specialization parameters
	        , class Params ///< shader push parameters structure
	        , template<class...> class Arrays ///< typelist of value types of array parameters
	        , class... Ts  ///< pack of value types of array parameters
	        >
	class Program<Specs, Params, Arrays<Ts...>> {
	public:
		Program(vuh::Device& device, const std::vector<char>& code
		        , vk::ShaderModuleCreateFlags flags
		        )
		   : _device(device)
		{
			_shader = device.createShaderModule(code, flags);
			auto dscTypes = typesToDscTypes<Arrays, Ts...>();
			auto sbo_descriptors_size = vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer
			                                                   , dscTypes.size());
			auto descriptor_sizes = std::vector<vk::DescriptorPoolSize>({sbo_descriptors_size}); // can be done compile-time, but not worth it
			_dscpool = device.createDescriptorPool(
			                        {vk::DescriptorPoolCreateFlags(), 1 // 1 here is the max number of descriptor sets that can be allocated from the pool
			                         , uint32_t(descriptor_sizes.size()), descriptor_sizes.data()
			                         }
			);
			auto bindings = dscTypesToLayout(dscTypes);
			_dsclayout = device.createDescriptorSetLayout(
			                                             {vk::DescriptorSetLayoutCreateFlags()
			                                              , uint32_t(bindings.size()), bindings.data()
			                                              }
			);
			_dscset = _device.allocateDescriptorSets({_dscpool, 1, &_dsclayout})[0];
			_pipecache = device.createPipeCache();
			auto push_constant_range = vk::PushConstantRange(vk::ShaderStageFlagBits::eCompute
			                                                 , 0, sizeof(Params));
			_pipelayout = device.createPipelineLayout(
						     {vk::PipelineLayoutCreateFlags(), 1, &_dsclayout, 1, &push_constant_range});
		}

		/// Destructor
		~Program() noexcept { release(); }

		Program(const Program&) = delete;
		Program& operator= (const Program&) = delete;

		/// Move constructor.
		Program(Program&& o) noexcept
		   : _shader(o._shader)
		   , _dscpool(o._dscpool)
		   , _dsclayout(o._dsclayout)
		   , _dscset(o._dscset)
		   , _pipecache(o._pipecache)
		   , _pipelayout(o._pipelayout)
		   , _pipeline(o._pipeline)
		   , _batch(o._batch)
		   , _device(o._device)
		{
			o._shader = nullptr; //
		}
		/// Move assignment.
		Program& operator= (Program&& o) noexcept {
			release();
			std::memcpy(this, &o, sizeof(Program));
			o._shader = nullptr;
			return *this;
		}

		/// Release resources associated with current Program.
		auto release() noexcept {
			if(_shader){
				_device.destroyShaderModule(_shader);
				_device.destroyDescriptorPool(_dscpool);
				_device.destroyDescriptorSetLayout(_dsclayout);
				_device.destroyPipelineCache(_pipecache);
				_device.destroyPipeline(_pipeline);
				_device.destroyPipelineLayout(_pipelayout);
			}
		}

		/// Specify running batch size (3D).
 		/// This only sets the dimensions of work batch in units of workgroup, does not start
		/// the actual calculation.
		auto batch(uint32_t x, uint32_t y = 1, uint32_t z = 1)-> Program& {
			_batch = {x, y, z};
			return *this;
		}
		
		/// Specify values of specification constants.
		/// Under the hood it creates the compute pipeline here.
		auto bind(const Specs& specs) const-> const Program& {
			// specialize constants of the shader
			auto specEntries = specs2mapentries(specs);
			auto specInfo = vk::SpecializationInfo(uint32_t(specEntries.size()), specEntries.data()
			                                       , sizeof(specs), &specs);
		
			// Specify the compute shader stage, and it's entry point (main), and specializations
			auto stageCI = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags()
			                                                 , vk::ShaderStageFlagBits::eCompute
			                                                 , _shader, "main", &specInfo);
			_pipeline = _device.createPipeline(_pipelayout, _pipecache, stageCI);
			return *this;
		}
		
		/// Associate buffers to binding points, and pushes the push constants.
		/// Sets up the command buffer. Programs is ready to be run.
		/// @pre Specs and batch sizes should be specified before calling this.
		auto bind(const Params& p, vuh::Array<Ts>&... args) const-> const Program& {
			constexpr auto N = sizeof...(args);
			auto dscinfos = std::array<vk::DescriptorBufferInfo, N>{{{args, 0, args.size_bytes()}... }}; // 0 is the offset here
			auto write_dscsets = dscinfos2writesets(_dscset, dscinfos, std::make_index_sequence<N>{});
			_device.updateDescriptorSets(write_dscsets, {}); // associate buffers to binding points in bindLayout

			// Start recording commands into the newly allocated command buffer.
			//	auto beginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit); // buffer is only submitted and used once
			auto beginInfo = vk::CommandBufferBeginInfo();
			_device._cmdbuf_compute.begin(beginInfo);

			// Before dispatch bind a pipeline, AND a descriptor set.
			// The validation layer will NOT give warnings if you forget those.
			_device._cmdbuf_compute.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);
			_device._cmdbuf_compute.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelayout
			                                 , 0, {_dscset}, {});

			_device._cmdbuf_compute.pushConstants(_pipelayout, vk::ShaderStageFlagBits::eCompute
			                                      , 0, sizeof(p), &p);

			_device._cmdbuf_compute.dispatch(_batch[0], _batch[1], _batch[2]); // start compute pipeline, execute the shader
			_device._cmdbuf_compute.end(); // end recording commands

			return *this;
		}

		/// Set the specialization constants, push the push parameters and associate buffers to binding points.
		/// @pre bacth sizes should be specified before calling this.
		auto bind(const Specs& specs, const Params& params, vuh::Array<Ts>&... args
		          ) const-> const Program&
		{
			bind(specs);
			return bind(params, args...);
		}

		/// Release device resources allocated by parameters binding.
		/// Should only be manually called before binding new set of parameters to same Program object
		auto unbind()-> void {
			_device.destroyDescriptorPool(_dscpool);
			_device.resetCommandPool(_device._cmdpool_compute, vk::CommandPoolResetFlags());
			auto dscTypes = typesToDscTypes<Arrays, Ts...>();
//			auto dscTypes = typesToDscTypes<Arrays, Ts...>();
			auto sbo_descriptors_size = vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer
			                                                   , dscTypes.size());
			auto descriptor_sizes = std::vector<vk::DescriptorPoolSize>({sbo_descriptors_size}); // can be done compile-time, but not worth it
			_dscpool = _device.createDescriptorPool(
			           {vk::DescriptorPoolCreateFlags(), 1 // 1 here is the max number of descriptor sets that can be allocated from the pool
			            , uint32_t(descriptor_sizes.size()), descriptor_sizes.data()
			            }
			);
		}

		/// Run the Program object on previously bound parameters, wait for completion.
		/// @pre bacth sizes should be specified before calling this.
		/// @pre all paramerters should be specialized, pushed and bound before calling this.
		auto run() const-> void {
			auto submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &_device.computeCmdBuffer()); // submit a single command buffer

			// submit the command buffer to the queue and set up a fence.
			auto queue = _device.computeQueue();
			auto fence = _device.createFence(vk::FenceCreateInfo()); // fence makes sure the control is not returned to CPU till command buffer is depleted
			queue.submit({submitInfo}, fence);
			_device.waitForFences({fence}, true, uint64_t(-1));      // -1 means wait for the fence indefinitely
			_device.destroyFence(fence);
		}

		/// Run program with provided parameters.
		/// @pre bacth sizes should be specified before calling this.
		auto operator()(const Specs& specs, const Params& params, vuh::Array<Ts>&... args
		                ) const-> void
		{
			bind(specs, params, args...);
			run();
		}
	private: // data
		vk::ShaderModule _shader;
		vk::DescriptorPool _dscpool;
		vk::DescriptorSetLayout _dsclayout;
		vk::DescriptorSet _dscset;
		vk::PipelineCache _pipecache;
		vk::PipelineLayout _pipelayout;
		mutable vk::Pipeline _pipeline;
		std::array<uint32_t, 3> _batch={0, 0, 0};
		vuh::Device& _device;
	}; // class Program
} // namespace vuh
