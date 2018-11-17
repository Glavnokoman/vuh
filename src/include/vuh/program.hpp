#pragma once

#include "array.hpp"
#include "device.h"
#include "utils.h"
#include "delayed.hpp"

#include <vulkan/vulkan.hpp>

#include <array>
#include <cstddef>
#include <tuple>
#include <utility>

namespace vuh {
	namespace detail {

		/// Traits to map array type to descriptor type
		template<class T> struct DictTypeToDsc {
			static constexpr auto value = T::descriptor_class;
		};

		/// @return tuple element offset
		template<size_t Idx, class T>
		constexpr auto tuple_element_offset(const T& tup)-> std::size_t {
			return size_t(reinterpret_cast<const char*>(&std::get<Idx>(tup))
		                 - reinterpret_cast<const char*>(&tup));
		}

		// helper
		template<class T, size_t... I>
		constexpr auto spec2entries(const T& specs, std::index_sequence<I...>
		                            )-> std::array<vk::SpecializationMapEntry, sizeof...(I)>
		{
			return {{ { uint32_t(I)
			          , uint32_t(tuple_element_offset<I>(specs))
			          , uint32_t(sizeof(typename std::tuple_element<I, T>::type))
			          }... }};
		}

		// helper
		template<class... Ts>
		auto typesToDscTypes()->std::array<vk::DescriptorType, sizeof...(Ts)> {
			return {detail::DictTypeToDsc<Ts>::value...};
		}

		// helper
		template<size_t N>
		auto dscTypesToLayout(const std::array<vk::DescriptorType, N>& dsc_types) {
			auto r = std::array<vk::DescriptorSetLayoutBinding, N>{};
			for(size_t i = 0; i < N; ++i){ // can be expanded compile-time
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

		// helper
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

		/// Transient command buffer data with a releaseable interface.
		struct ComputeBuffer {
			/// Constructor. Takes ownership over provided buffer.
			ComputeBuffer(vuh::Device& device, vk::CommandBuffer buffer)
			   : cmd_buffer(buffer), device(&device){}

			/// Release resources associated with owned command buffer.
			/// Buffer is released from device's compute command pool.
			auto release() noexcept-> void {
				if(device){
					device->freeCommandBuffers(device->computeCmdPool(), 1, &cmd_buffer);
				}
			}
		public: // data
			vk::CommandBuffer cmd_buffer; ///< command buffer to submit async computation commands
			std::unique_ptr<vuh::Device, util::NoopDeleter<vuh::Device>> device; ///< underlying device
		}; // struct ComputeData

		/// Helper class for use as a Delayed<> parameter extending the lifetime of the command
		/// buffer and a noop triggered action.
		struct Compute: private util::Resource<ComputeBuffer> {
			/// Constructor
			explicit Compute(vuh::Device& device, vk::CommandBuffer buffer)
			   : Resource<ComputeBuffer>(device, std::move(buffer))
			{}

			/// Noop. Action to be triggered when the fence is signaled.
			constexpr auto operator()() noexcept-> void {}
		}; // struct Compute

		/// Program base functionality.
		/// Initializes and keeps most state variables, and array argument handling building blocks.
		class ProgramBase {
		public:
			/// Run the Program object on previously bound parameters, wait for completion.
			/// @pre bacth sizes should be specified before calling this.
			/// @pre all paramerters should be specialized, pushed and bound before calling this.
			auto run()-> void {
				auto submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &_device.computeCmdBuffer()); // submit a single command buffer

				// submit the command buffer to the queue and set up a fence.
				auto queue = _device.computeQueue();
				queue.submit({submitInfo}, nullptr);
				queue.waitIdle();
			}

			/// Run the Program object on previously bound parameters.
			/// @return Delayed<Compute> object used for synchronization with host
			auto run_async()-> vuh::Delayed<Compute> {
				auto buffer = _device.releaseComputeCmdBuffer();
				auto submitInfo = vk::SubmitInfo(0, nullptr, nullptr, 1, &buffer); // submit a single command buffer

				// submit the command buffer to the queue and set up a fence.
				auto queue = _device.computeQueue();
				auto fence = _device.createFence(vk::FenceCreateInfo()); // fence makes sure the control is not returned to CPU till command buffer is depleted
				queue.submit({submitInfo}, fence);

				return Delayed<Compute>{fence, _device, Compute(_device, buffer)};
			}
		protected:
			/// Construct object using given a vuh::Device and path to SPIR-V shader code.
			ProgramBase(vuh::Device& device        ///< device used to run the code
			            , const char* filepath     ///< file path to SPIR-V shader code
			            , vk::ShaderModuleCreateFlags flags={}
			            )
				: ProgramBase(device, read_spirv(filepath), flags)
			{}

			/// Construct object using given a vuh::Device a SPIR-V shader code.
			ProgramBase(vuh::Device& device              ///< device used to run the code
			            , const std::vector<char>& code  ///< actual binary SPIR-V shader code
			            , vk::ShaderModuleCreateFlags flags={}
			            )
			   : _device(device)
			{
				_shader = device.createShaderModule({ flags, uint32_t(code.size())
				                                    , reinterpret_cast<const uint32_t*>(code.data())
				                                    });
			}

			/// Destroy the object and release associated resources.
			~ProgramBase() noexcept {
				release();
			}

			ProgramBase(const ProgramBase&) = delete;
			ProgramBase& operator= (const ProgramBase&) = delete;

			/// Move constructor.
			ProgramBase(ProgramBase&& o) noexcept
			   : _shader(o._shader)
			   , _dsclayout(o._dsclayout)
			   , _dscpool(o._dscpool)
			   , _dscset(o._dscset)
			   , _pipecache(o._pipecache)
			   , _pipelayout(o._pipelayout)
			   , _pipeline(o._pipeline)
			   , _device(o._device)
			   , _batch(o._batch)
			{
				o._shader = nullptr; //
			}

			/// Move assignment.
			ProgramBase& operator= (ProgramBase&& o) noexcept {
				release();
				std::memcpy(this, &o, sizeof(ProgramBase));
				o._shader = nullptr;
				return *this;
			}

			/// Release resources associated with current object.
			auto release() noexcept-> void {
				if(_shader){
					_device.destroyShaderModule(_shader);
					_device.destroyDescriptorPool(_dscpool);
					_device.destroyDescriptorSetLayout(_dsclayout);
					_device.destroyPipelineCache(_pipecache);
					_device.destroyPipeline(_pipeline);
					_device.destroyPipelineLayout(_pipelayout);
				}
			}

			/// Initialize the pipeline.
			/// Creates descriptor set layout, pipeline cache and the pipeline layout.
			template<size_t N, class... Arrs>
			auto init_pipelayout(const std::array<vk::PushConstantRange, N>& psrange, Arrs&...)-> void {
				auto dscTypes = typesToDscTypes<Arrs...>();
				auto bindings = dscTypesToLayout(dscTypes);
				_dsclayout = _device.createDescriptorSetLayout(
				                                       { vk::DescriptorSetLayoutCreateFlags()
				                                       , uint32_t(bindings.size()), bindings.data()
				                                       });
				_pipecache = _device.createPipelineCache({});
				_pipelayout = _device.createPipelineLayout(
				        {vk::PipelineLayoutCreateFlags(), 1, &_dsclayout, uint32_t(N), psrange.data()});
			}

			/// Allocates descriptors sets
			template<class... Arrs>
			auto alloc_descriptor_sets(Arrs&...)-> void {
				assert(_dsclayout);
				auto sbo_descriptors_size = vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer
				                                                   , sizeof...(Arrs));
				auto descriptor_sizes = std::array<vk::DescriptorPoolSize, 1>({sbo_descriptors_size}); // can be done compile-time, but not worth it
				_dscpool = _device.createDescriptorPool(
				                             {vk::DescriptorPoolCreateFlags(), 1 // 1 here is the max number of descriptor sets that can be allocated from the pool
				                              , uint32_t(descriptor_sizes.size()), descriptor_sizes.data()
				                              });
				_dscset = _device.allocateDescriptorSets({_dscpool, 1, &_dsclayout})[0];
			}

			/// Starts writing to the device's compute command buffer.
			/// Binds a pipeline and a descriptor set.
			template<class... Arrs>
			auto command_buffer_begin(Arrs&... arrs)-> void {
				assert(_pipeline); /// pipeline supposed to be initialized before this

				constexpr auto N = sizeof...(arrs);
				auto dscinfos = std::array<vk::DescriptorBufferInfo, N>{
					                           {{arrs.buffer()
				                               , arrs.offset()*sizeof(typename Arrs::value_type)
				                               , arrs.size_bytes()}... }
				                };
				auto write_dscsets = dscinfos2writesets(_dscset, dscinfos
				                                       , std::make_index_sequence<N>{});
				_device.updateDescriptorSets(write_dscsets, {}); // associate buffers to binding points in bindLayout

				// Start recording commands into the newly allocated command buffer.
				//	auto beginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit); // buffer is only submitted and used once
				auto cmdbuf = _device.computeCmdBuffer();
				auto beginInfo = vk::CommandBufferBeginInfo();
				cmdbuf.begin(beginInfo);

				// Before dispatch bind a pipeline, AND a descriptor set.
				cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, _pipeline);
				cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, _pipelayout
				                          , 0, {_dscset}, {});
			}

			/// Ends command buffer creation. Writes dispatch info and signals end of commands recording.
			auto command_buffer_end()-> void {
				auto cmdbuf = _device.computeCmdBuffer();
				cmdbuf.dispatch(_batch[0], _batch[1], _batch[2]); // start compute pipeline, execute the shader
				cmdbuf.end(); // end recording commands
			}
		protected: // data
			vk::ShaderModule _shader;            ///< compute shader to execute
			vk::DescriptorSetLayout _dsclayout;  ///< descriptor set layout. This defines the kernel's array parameters interface.
			vk::DescriptorPool _dscpool;         ///< descitptor ses pool. Descriptors are allocated on this pool.
			vk::DescriptorSet _dscset;           ///< descriptors set
			vk::PipelineCache _pipecache;        ///< pipeline cache
			vk::PipelineLayout _pipelayout;      ///< pipeline layout
			mutable vk::Pipeline _pipeline;      ///< pipeline itself

			vuh::Device& _device;                ///< refer to device to run shader on
			std::array<uint32_t, 3> _batch={0, 0, 0}; ///< 3D evaluation grid dimensions (number of workgroups to run)
		}; // class ProgramBase

		/// Part of Program handling specialization constants.
		/// Keeps the specialization constants state
		/// (between its set by Program::spec() call and actually communicated to device).
		/// Does the pipeline init.
		template<class Specs> class SpecsBase;

		/// Explicit specialization for non-empty specialization constants interface.
		template<template<class...> class Specs, class... Spec_Ts>
		class SpecsBase<Specs<Spec_Ts...>>: public ProgramBase {
		protected:
			/// Construct object using given a vuh::Device and path to SPIR-V shader code.
			SpecsBase(Device& device, const char* filepath, vk::ShaderModuleCreateFlags flags={})
			   : ProgramBase(device, filepath, flags)
			{}

			/// Construct object using given a vuh::Device a SPIR-V shader code.
			SpecsBase(Device& device, const std::vector<char>& code, vk::ShaderModuleCreateFlags f={})
			   : ProgramBase(device, code, f)
			{}

			/// Initialize the pipeline.
			/// Specialization constants interface is defined here.
			auto init_pipeline()-> void {
				auto specEntries = specs2mapentries(_specs);
				auto specInfo = vk::SpecializationInfo(uint32_t(specEntries.size()), specEntries.data()
																	, sizeof(_specs), &_specs);

				// Specify the compute shader stage, and it's entry point (main), and specializations
				auto stageCI = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags()
																				 , vk::ShaderStageFlagBits::eCompute
																				 , _shader, "main", &specInfo);
				_pipeline = _device.createPipeline(_pipelayout, _pipecache, stageCI);
			}
		protected:
			std::tuple<Spec_Ts...> _specs; ///< hold the state of specialization constants between call to specs() and actual pipeline creation
		};

		/// Explicit specialization for empty specialization constants interface.
		template<>
		class SpecsBase<typelist<>>: public ProgramBase{
		protected:
			/// Construct object using given a vuh::Device and path to SPIR-V shader code.
			SpecsBase(Device& device, const char* filepath, vk::ShaderModuleCreateFlags flags={})
			   : ProgramBase(device, filepath, flags)
			{}

			/// Construct object using given a vuh::Device a SPIR-V shader code.
			SpecsBase(Device& device, const std::vector<char>& code, vk::ShaderModuleCreateFlags f={})
			   : ProgramBase(device, code, f)
			{}

			/// Initialize the pipeline with empty specialialization constants interface.
			auto init_pipeline()-> void {
				auto stageCI = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags()
																				 , vk::ShaderStageFlagBits::eCompute
																				 , _shader, "main", nullptr);

				_pipeline = _device.createPipeline(_pipelayout, _pipecache, stageCI);
			}
		}; // class SpecsBase
	} // namespace detail

	/// Actually runnable entity. Before array parameters are bound (and program run)
	/// working grid dimensions should be set up, and if there are specialization constants to set
	/// they should be set before that too.
	/// Push constants can be specified together with the input/output array parameters right at the
	/// calling point.
	/// Or alternatively the empty call operator may be triggered on previously fully set up instance
	/// of Program.
	template<class Specs=typelist<>, class Params=typelist<>> class Program;

	/// Specialization with non-empty specialization constants and push constants.
	template<template<class...> class Specs, class... Specs_Ts , class Params>
	class Program<Specs<Specs_Ts...>, Params>: public detail::SpecsBase<Specs<Specs_Ts...>> {
		using Base = detail::SpecsBase<Specs<Specs_Ts...>>;
	public:
		/// Initialize program on a device using SPIR-V code at a given path
		Program(vuh::Device& device, const char* filepath, vk::ShaderModuleCreateFlags flags={})
		   : Base(device, read_spirv(filepath), flags)
		{}

		/// Initialize program on a device from binary SPIR-V code
		Program(vuh::Device& device, const std::vector<char>& code
		        , vk::ShaderModuleCreateFlags flags={}
		        )
		   : Base(device, code, flags)
		{}

		using Base::run;
		using Base::run_async;

		/// Specify running batch size (3D).
 		/// This only sets the dimensions of work batch in units of workgroup, does not start
		/// the actual calculation.
		auto grid(uint32_t x, uint32_t y = 1, uint32_t z = 1)-> Program& {
			Base::_batch = {x, y, z};
			return *this;
		}

		/// Specify values of specification constants.
		auto spec(Specs_Ts... specs)-> Program& {
			Base::_specs = std::make_tuple(specs...);
			return *this;
		}

		/// Associate buffers to binding points, and pushes the push constants.
		/// Does most of setup here. Program is ready to be run.
		/// @pre Grid dimensions and specialization constants (if applicable)
		/// should be specified before calling this.
		template<class... Arrs>
		auto bind(const Params& p, Arrs&&... args)-> const Program& {
			if(!Base::_pipeline){ // handle multiple rebind
				init_pipelayout(args...);
				Base::alloc_descriptor_sets(args...);
				Base::init_pipeline();
			}
			create_command_buffer(p, args...);
			return *this;
		}

		/// Run program with provided parameters.
		/// @pre grid dimensions should be specified before calling this.
		template<class... Arrs>
		auto run(const Params& params, Arrs&&... args)-> void {
			bind(params, args...);
			Base::run();
		}

		/// Run program with provided parameters.
		/// @pre grid dimensions should be specified before calling this.
		template<class... Arrs>
		auto operator()(const Params& params, Arrs&&... args)-> void {
			bind(params, args...);
			Base::run();
		}

		/// Initiate execution of the program with provided parameters and immidiately return.
		/// @return Delayed<Compute> object for synchronization with host.
		/// @pre grid dimensions should be specified before callind this.
		template<class... Arrs>
		auto run_async(const Params& params, Arrs&&... args)-> vuh::Delayed<detail::Compute> {
			bind(params, args...);
			return Base::run_async();
		}
	private: // helpers
		/// Set up the state of the kernel that depends on number and types of bound array parameters.
		/// Initizalizes the pipeline layout, declares the push constants interface.
		template<class... Arrs>
		auto init_pipelayout(Arrs&... args)-> void {
			auto psranges = std::array<vk::PushConstantRange, 1>{{
					vk::PushConstantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(Params))}};
			Base::init_pipelayout(psranges, args...);
		}

		/// Populate the associated device's compute command buffer.
		/// Binds the descriptors and pushes the push constants.
		template<class... Arrs>
		auto create_command_buffer(const Params& p, Arrs&... args)-> void {
			Base::command_buffer_begin(args...);
			Base::_device.computeCmdBuffer().pushConstants(Base::_pipelayout
			                           , vk::ShaderStageFlagBits::eCompute , 0, sizeof(p), &p);
			Base::command_buffer_end();
		}
	}; // class Program

	/// Specialization with non-empty specialization constants and empty push constants.
	template<template<class...> class Specs, class... Specs_Ts>
	class Program<Specs<Specs_Ts...>, typelist<>>: public detail::SpecsBase<Specs<Specs_Ts...>>{
		using Base = detail::SpecsBase<Specs<Specs_Ts...>>;
	public:
		/// Initialize program on a device using SPIR-V code at a given path
		Program(vuh::Device& device, const char* filepath, vk::ShaderModuleCreateFlags flags={})
		   : Base(device, filepath, flags)
		{}

		/// Initialize program on a device from binary SPIR-V code
		Program(vuh::Device& device, const std::vector<char>& code
		        , vk::ShaderModuleCreateFlags flags={}
		        )
		   : Base (device, code, flags)
		{}

		using Base::run;
		using Base::run_async;

		/// Specify (3D) running batch size.
 		/// This only sets the dimensions of work batch in units of workgroup, does not start
		/// the actual calculation.
		auto grid(uint32_t x, uint32_t y = 1, uint32_t z = 1)-> Program& {
			Base::_batch = {x, y, z};
			return *this;
		}

		/// Specify values of specification constants.
		auto spec(Specs_Ts... specs)-> Program& {
			Base::_specs = std::make_tuple(specs...);
			return *this;
		}

		/// Associate buffers to binding points, and pushes the push constants.
		/// Does most of setup here. Program is ready to be run.
		/// @pre Grid dimensions and specialization constants (if applicable)
		/// should be specified before calling this.
		template<class... Arrs>
		auto bind(Arrs&&... args)-> const Program& {
			if(!Base::_pipeline){ // handle multiple rebind
				Base::init_pipelayout(std::array<vk::PushConstantRange, 0>{}, args...);
				Base::alloc_descriptor_sets(args...);
				Base::init_pipeline();
			}
			Base::command_buffer_begin(args...);
			Base::command_buffer_end();
			return *this;
		}

		/// Run program with provided parameters.
		/// @pre grid dimensions should be specified before calling this.
		template<class... Arrs>
		auto operator()(Arrs&&... args)-> void {
			bind(args...);
			Base::run();
		}

		/// Initiate execution of the program with provided parameters and immidiately return.
		/// @return Delayed<Compute> object for synchronization with host.
		/// @pre grid dimensions should be specified before callind this.
		template<class... Arrs>
		auto run_async(Arrs&&... args)-> vuh::Delayed<detail::Compute> {
			bind(args...);
			return Base::run_async();
		}
	}; // class Program
} // namespace vuh
