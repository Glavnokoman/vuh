#pragma once

#include "array.hpp"
#include "device.h"
#include "utils.h"

#include <vulkan/vulkan.hpp>

#include <array>
#include <cstddef>
#include <tuple>
#include <utility>

namespace vuh {
	namespace detail {
		template<class T> struct DictTypeToDsc;

		template<class T> struct DictTypeToDsc<vuh::Array<T>>{
			static constexpr vk::DescriptorType value = vk::DescriptorType::eStorageBuffer;
		};

		// @return tuple element offset
		template<size_t Idx, class T>
		constexpr auto tuple_element_offset(const T& tup)-> std::size_t {
			return size_t(reinterpret_cast<const char*>(&std::get<Idx>(tup))
		                 - reinterpret_cast<const char*>(&tup));
		}

		//
		template<class T, size_t... I>
		constexpr auto spec2entries(const T& specs, std::index_sequence<I...>
		                            )-> std::array<vk::SpecializationMapEntry, sizeof...(I)>
		{
			return {{ { uint32_t(I)
			          , uint32_t(tuple_element_offset<I>(specs))
			          , uint32_t(sizeof(typename std::tuple_element<I, T>::type))
			          }... }};
		}

		/// doc me
		template<class... Ts>
		auto typesToDscTypes()->std::array<vk::DescriptorType, sizeof...(Ts)> {
			return {detail::DictTypeToDsc<Ts>::value...};
		}

		/// doc me
		template<size_t N>
		auto dscTypesToLayout(const std::array<vk::DescriptorType, N>& dsc_types) {
			auto r = std::array<vk::DescriptorSetLayoutBinding, N>{};
			for(size_t i = 0; i < N; ++i){ // can be done compile-time
				r[i] = {uint32_t(i), dsc_types[i], 1, vk::ShaderStageFlagBits::eCompute};
			}
			return r;
		}

		/// @return specialization map array
		template<class... Ts>
		auto specs2mapentries(const std::tuple<Ts...>& specs
		                      )-> std::array<vk::SpecializationMapEntry, sizeof...(Ts)>
		{
			return spec2entries(specs, std::make_index_sequence<sizeof...(Ts)>{});
		}

		/// doc me
		template<class T, size_t... I>
		auto dscinfos2writesets(vk::DescriptorSet dscset, const T& infos
		                        , std::index_sequence<I...>
		                        )-> std::array<vk::WriteDescriptorSet, sizeof...(I)>
		{
			auto r = std::array<vk::WriteDescriptorSet, sizeof...(I)>{{
				{dscset, uint32_t(I), 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &infos[I]}...
			}};
			return r;
		}
	} // namespace detail

	/// doc me
	template<class Specs, class Params> class Program;

	/// specialization to unpack array types parameters
	template<template<class...> class Specs, class... Specs_Ts
	         , class Params  ///< shader push parameters structure
	         >
	class Program<Specs<Specs_Ts...>, Params> {
	public:

		/// Initialize program on a device using spirv code at a given path
		Program(vuh::Device& device, const char* filepath, vk::ShaderModuleCreateFlags flags={})
		   : Program(device, read_spirv(filepath), flags)
		{}

		/// Initialize program on a device from binary spirv code
		Program(vuh::Device& device, const std::vector<char>& code
		        , vk::ShaderModuleCreateFlags flags={}
		        )
		   : _device(device)
		{
			_shader = device.createShaderModule(code, flags);
		}

		/// Destructor
		~Program() noexcept { release(); }

		Program(const Program&) = delete;
		Program& operator= (const Program&) = delete;

		/// Move constructor.
		Program(Program&& o) noexcept
		   : _shader(o._shader)
		   , _dsclayout(o._dsclayout)
		   , _dscpool(o._dscpool)
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
		auto grid(uint32_t x, uint32_t y = 1, uint32_t z = 1)-> Program& {
			_batch = {x, y, z};
			return *this;
		}
		
		/// Specify values of specification constants.
		auto spec(Specs_Ts... specs)-> Program& {
			_specs = {specs...};
			return *this;
		}
		
		/// Associate buffers to binding points, and pushes the push constants.
		/// Does most of setup here. Programs is ready to be run.
		/// @pre Specs and batch sizes should be specified before calling this.
		template<class... Arrs>
		auto bind(const Params& p, Arrs&... args)-> const Program& {
			init_pipelayout(args...);
			alloc_descriptor_sets();
			init_pipeline();
			create_command_buffer(p, args...);
			return *this;
		}

		/// Run the Program object on previously bound parameters, wait for completion.
		/// @pre bacth sizes should be specified before calling this.
		/// @pre all paramerters should be specialized, pushed and bound before calling this.
		auto run()-> void {
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

		template<class... Arrs>
		auto operator()(const Params& params, Arrs&... args)-> void {
			bind(params, args...);
			run();
		}
	private: // helpers
		/// set up the state of the kernel that depends on number and types of bound array parameters
		template<class... Arrs>
		auto init_pipelayout(Arrs&...)-> void {
			_num_sbo_params = sizeof...(Arrs);
			auto dscTypes = detail::typesToDscTypes<Arrs...>();
			auto bindings = detail::dscTypesToLayout(dscTypes);
			_dsclayout = _device.createDescriptorSetLayout(
			                         { vk::DescriptorSetLayoutCreateFlags()
			                         , uint32_t(bindings.size()), bindings.data()
			                         }
			);
			_pipecache = _device.createPipelineCache({});
			auto push_constant_range = vk::PushConstantRange(vk::ShaderStageFlagBits::eCompute
			                                                 , 0, sizeof(Params));
			_pipelayout = _device.createPipelineLayout(
						     {vk::PipelineLayoutCreateFlags(), 1, &_dsclayout, 1, &push_constant_range});
		}

		///
		auto alloc_descriptor_sets()-> void {
			assert(_dsclayout);
			if(_dscpool){ // unbind previously bound descriptor sets if any
				_device.destroyDescriptorPool(_dscpool);
				_device.resetCommandPool(_device.computeCmdPool(), vk::CommandPoolResetFlags());
			}

			auto sbo_descriptors_size = vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer
			                                                   , _num_sbo_params);
			auto descriptor_sizes = std::array<vk::DescriptorPoolSize, 1>({sbo_descriptors_size}); // can be done compile-time, but not worth it
			_dscpool = _device.createDescriptorPool(
			                        {vk::DescriptorPoolCreateFlags(), 1 // 1 here is the max number of descriptor sets that can be allocated from the pool
			                         , uint32_t(descriptor_sizes.size()), descriptor_sizes.data()
			                         }
			);
			_dscset = _device.allocateDescriptorSets({_dscpool, 1, &_dsclayout})[0];
		}

		///
		auto init_pipeline()-> void {
			if(sizeof...(Specs_Ts) == 0){ // no specialization constants
				// Specify the compute shader stage, and it's entry point (main), and specializations
				auto stageCI = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags()
				                                                 , vk::ShaderStageFlagBits::eCompute
				                                                 , _shader, "main", nullptr);

				_pipeline = _device.createPipeline(_pipelayout, _pipecache, stageCI);
			} else {
				auto specEntries = detail::specs2mapentries(_specs);
				auto specInfo = vk::SpecializationInfo(uint32_t(specEntries.size()), specEntries.data()
				                                       , sizeof(_specs), &_specs);

				// Specify the compute shader stage, and it's entry point (main), and specializations
				auto stageCI = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags()
				                                                 , vk::ShaderStageFlagBits::eCompute
				                                                 , _shader, "main", &specInfo);
				_pipeline = _device.createPipeline(_pipelayout, _pipecache, stageCI);
			}
		}

		///
		template<class... Arrs>
		auto create_command_buffer(const Params& p, Arrs&... args)-> void {
			assert(_pipeline); /// pipeline supposed to be initialized before this

			constexpr auto N = sizeof...(args);
			auto dscinfos = std::array<vk::DescriptorBufferInfo, N>{{{args, 0, args.size_bytes()}... }}; // 0 is the offset here
			auto write_dscsets = detail::dscinfos2writesets(_dscset, dscinfos
			                                                , std::make_index_sequence<N>{});
			_device.updateDescriptorSets(write_dscsets, {}); // associate buffers to binding points in bindLayout

			// Start recording commands into the newly allocated command buffer.
			//	auto beginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit); // buffer is only submitted and used once
			auto cmdbuf = _device.computeCmdBuffer();
			auto beginInfo = vk::CommandBufferBeginInfo();
			cmdbuf.begin(beginInfo);

			// Before dispatch bind a pipeline, AND a descriptor set.
			// The validation layer will NOT give warnings if you forget those.
			cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);
			cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelayout, 0, {_dscset}, {});

			cmdbuf.pushConstants(_pipelayout, vk::ShaderStageFlagBits::eCompute , 0, sizeof(p), &p);

			cmdbuf.dispatch(_batch[0], _batch[1], _batch[2]); // start compute pipeline, execute the shader
			cmdbuf.end(); // end recording commands
		}
	private: // data
		vk::ShaderModule _shader;
		vk::DescriptorSetLayout _dsclayout;
		vk::DescriptorPool _dscpool;
		vk::DescriptorSet _dscset;
		vk::PipelineCache _pipecache;
		vk::PipelineLayout _pipelayout;
		mutable vk::Pipeline _pipeline;

		std::array<uint32_t, 3> _batch={0, 0, 0};
		std::tuple<Specs_Ts...> _specs; ///< hold the state of specialization constants between call to specs() and actual pipeline creation
		std::size_t _num_sbo_params;

		vuh::Device& _device;
	}; // class Program
} // namespace vuh
