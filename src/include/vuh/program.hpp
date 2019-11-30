#pragma once

#include "vuh/core/core.hpp"
#include "array.hpp"
#include "device.h"
#include "utils.h"
#include "delayed.hpp"
#include <array>
#include <cstddef>
#include <tuple>
#include <utility>
#include <map>

namespace vuh {
	namespace detail {

		/// @return tuple element offset
		template<size_t Idx, class T>
		constexpr auto tuple_element_offset(const T& tup)-> std::size_t {
			return size_t(reinterpret_cast<const char*>(&std::get<Idx>(tup))
		                 - reinterpret_cast<const char*>(&tup));
		}

		// helper
		template<class T, size_t... I>
		constexpr auto spec2entries(const T& specs, std::index_sequence<I...>
		                            )-> std::array<vhn::SpecializationMapEntry, sizeof...(I)>
		{
			return {{ { uint32_t(I)
			          , uint32_t(tuple_element_offset<I>(specs))
			          , uint32_t(sizeof(typename std::tuple_element<I, T>::type))
			          }... }};
		}

		// helper
		template<size_t N>
		auto dscTypesToLayout(const std::array<vhn::DescriptorType, N>& dsc_types) {
			auto r = std::array<vhn::DescriptorSetLayoutBinding, N>{};
			for(size_t i = 0; i < N; ++i){ // can be expanded compile-time
				r[i] = {uint32_t(i), dsc_types[i], 1, vhn::ShaderStageFlagBits::eCompute};
			}
			return r;
		}

		/// @return specialization map array
		template<class... Ts>
		auto specs2mapentries(const std::tuple<Ts...>& specs
		                      )-> std::array<vhn::SpecializationMapEntry, sizeof...(Ts)>
		{
			return spec2entries(specs, std::make_index_sequence<sizeof...(Ts)>{});
		}

		// helper
		template<class T, size_t... I>
		auto dscinfos2writesets(vhn::DescriptorSet dsc_set, T& infos
		                        , std::index_sequence<I...>
		                        )-> std::array<vhn::WriteDescriptorSet, sizeof...(I)>
		{
			std::array<vhn::WriteDescriptorSet, sizeof...(I)> r;
			for(size_t i = 0; i < r.size(); i++ ) {
				r[i].setDstSet(dsc_set);
				r[i].setDstBinding(uint32_t(i));
				r[i].setDstArrayElement(0);
				r[i].setDescriptorCount(1);
				r[i].setDescriptorType(infos[i]->descriptorType());
				if(vhn::DescriptorType::eStorageImage == infos[i]->descriptorType()) {
					r[i].setPImageInfo(&(infos[i]->descriptorImageInfo()));
				} else if (vhn::DescriptorType::eStorageBuffer == infos[i]->descriptorType()) {
					r[i].setPBufferInfo(&(infos[i]->descriptorBufferInfo()));
				} else if(vhn::DescriptorType::eCombinedImageSampler == infos[i]->descriptorType()) {
					r[i].setPImageInfo(&(infos[i]->descriptorImageInfo()));
				}
			}
			return r;
		}

		/// Transient command buffer data with a releaseable interface.
		struct ComputeBuffer {
			/// Constructor. Takes ownership over provided buffer.
			ComputeBuffer(vuh::Device& dev, vhn::CommandBuffer buffer)
			   : cmd_buffer(buffer), _dev(&dev){}

			/// Release resources associated with owned command buffer.
			/// Buffer is released from device's compute command pool.
			auto release() noexcept-> void {
				if(_dev){
					_dev->freeCommandBuffers(_dev->computeCmdPool(), 1, &cmd_buffer);
				}
			}
		public: // data
			vhn::CommandBuffer cmd_buffer; ///< command buffer to submit async computation commands
			std::unique_ptr<vuh::Device, util::NoopDeleter<vuh::Device>> _dev; ///< underlying device
		}; // struct ComputeData

		/// Helper class for use as a Delayed<> parameter extending the lifetime of the command
		/// buffer and a noop triggered action.
		struct Compute: private util::Resource<ComputeBuffer> {
			/// Constructor
			explicit Compute(vuh::Device& device, vhn::CommandBuffer buffer)
			   : Resource<ComputeBuffer>(device, std::move(buffer))
			{}

			/// Noop. Action to be triggered when the fence is signaled.
			constexpr auto operator()() noexcept-> void {}
		}; // struct Compute

		/// Program base functionality.
		/// Initializes and keeps most state variables, and array argument handling building blocks.
		class ProgramBase : virtual public vuh::core {
		public:
			/// Run the Program object on previously bound parameters, wait for completion.
			/// @pre bacth sizes should be specified before calling this.
			/// @pre all paramerters should be specialized, pushed and bound before calling this.
			auto run()-> void {
				auto submitInfo = vhn::SubmitInfo(0, nullptr, nullptr, 1, &_dev.computeCmdBuffer()); // submit a single command buffer

				// submit the command buffer to the queue and set up a fence.
				auto queue = _dev.computeQueue();
				queue.submit({submitInfo}, nullptr);
				queue.waitIdle();
			}

			/// Run the Program object on previously bound parameters.
			/// if suspend == true we will add an event on the top of command queue ,the command will wait until event is fired
			/// we use this for precise time measurement or command pool
			/// we submit some tasks to driver and wake up them when we need
			/// it's not thread safe , please call resume on the thread who create the program
			/// do'nt wait for too long time ,as we know timeout may occur about 2-3 seconds later on android
			/// @return Delayed<Compute> object used for synchronization with host
			auto run_async(bool suspend=false)-> vuh::Delayed<Compute> {
				vhn::Result res = vhn::Result::eSuccess;
				vuh::Event ev;
				auto buffer = _dev.releaseComputeCmdBuffer(res);
				if (vhn::Result::eSuccess == res) {
					auto submitInfo = vhn::SubmitInfo(0, nullptr, nullptr, 1,
													 &buffer); // submit a single command buffer
					if (suspend) {
						ev = vuh::Event(_dev);
						if (bool(ev)) {
							buffer.waitEvents(1, &ev, vhn::PipelineStageFlagBits::eHost,
											  vhn::PipelineStageFlagBits::eTopOfPipe, 0, NULL, 0,
											  NULL,
											  0,
											  NULL);
						}
					}
					if ((!suspend) || bool(ev)) {
						// submit the command buffer to the queue and set up a fence.
						auto queue = _dev.computeQueue();
						vuh::Fence fence(_dev);  // fence makes sure the control is not returned to CPU till command buffer is deplet
						if (bool(fence)) {
							queue.submit({submitInfo}, fence);

							return Delayed<Compute>{fence, ev, _dev,
													Compute(_dev, buffer)};
						}
					}
				}
				return Delayed<Compute>(_dev, res, ev,Compute(_dev, nullptr));
			}

			explicit operator bool() const { return bool(_dev); };
			bool operator!() const {return !_dev; };
		protected:
			/// Construct object using given a vuh::Device and path to SPIR-V shader code.
			ProgramBase(vuh::Device& dev        ///< device used to run the code
			            , const char* filepath     ///< file path to SPIR-V shader code
			            , vhn::ShaderModuleCreateFlags flags={}
			            )
				: ProgramBase(dev, read_spirv(filepath), flags)
			{}

			/// Construct object using given a vuh::Device a SPIR-V shader code.
			ProgramBase(vuh::Device& dev              ///< device used to run the code
			            , const std::vector<char>& code  ///< actual binary SPIR-V shader code
			            , vhn::ShaderModuleCreateFlags flags={}
			            )
			   : _dev(dev)
			{
				auto shader = _dev.createShaderModule({ flags, uint32_t(code.size())
																 , reinterpret_cast<const uint32_t*>(code.data())
														 });
#ifdef VULKAN_HPP_NO_EXCEPTIONS
				_res = shader.result;
				VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _res);
				_shader = shader.value;
#else
				_shader = shader;
#endif
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
			   , _dev(o._dev)
			   , _batch(o._batch)
			{
				o._shader = nullptr; //
			}

			/// Move assignment. Releases resources allocated for current instance before taking 
			/// ownership over those of the other instance.
			ProgramBase& operator= (ProgramBase&& o) noexcept {
				release();
				// member-wise copy
				_shader     = o._shader;
				_dsclayout  = o._dsclayout;
				_dscpool    = o._dscpool;
				_dscset     = o._dscset;
				_pipecache  = o._pipecache; 
				_pipelayout	= o._pipelayout;
				_pipeline   = o._pipeline;
				_dev     	= o._dev;
				_batch      = o._batch;	
			
				o._shader = nullptr;
				return *this;
			}

			/// Release resources associated with current object.
			auto release() noexcept-> void {
				if(_shader){
					_dev.destroyShaderModule(_shader);
					_dev.destroyDescriptorPool(_dscpool);
					_dev.destroyDescriptorSetLayout(_dsclayout);
					_dev.destroyPipelineCache(_pipecache);
					_dev.destroyPipeline(_pipeline);
					_dev.destroyPipelineLayout(_pipelayout);
				}
			}

			/// Initialize the pipeline.
			/// Creates descriptor set layout, pipeline cache and the pipeline layout.
			template<size_t N, class... Arrs>
			auto init_pipelayout(const std::array<vhn::PushConstantRange, N>& psrange, Arrs&... args)-> void {
				constexpr auto M = sizeof...(Arrs);
				auto arr_mem = std::array<vuh::mem::BasicMemory*, M>{ &args ... };
				std::array<vhn::DescriptorType, M> dscTypes;
				for(size_t i = 0; i < M ; i ++) {
					dscTypes[i] = arr_mem[i]->descriptorType();
				}

				auto bindings = dscTypesToLayout(dscTypes);
				auto layout = _dev.createDescriptorSetLayout(
				                                       { vhn::DescriptorSetLayoutCreateFlags()
				                                       , uint32_t(bindings.size()), bindings.data()
				                                       });
#ifdef VULKAN_HPP_NO_EXCEPTIONS
				_res = layout.result;
				VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _res);
				_dsclayout = layout.value;
#else
				_dsclayout = layout;
#endif
				if(bool(_dsclayout)) {
					auto pipe_cache = _dev.createPipelineCache({});
#ifdef VULKAN_HPP_NO_EXCEPTIONS
					_res = pipe_cache.result;
					VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _res);
					_pipecache = pipe_cache.value;
#else
					_pipecache = pipe_cache;
#endif
				}
				if(bool(_pipecache)) {
					auto pipe_layout = _dev.createPipelineLayout(
							{vhn::PipelineLayoutCreateFlags(), 1, &_dsclayout, uint32_t(N),
							 psrange.data()});
#ifdef VULKAN_HPP_NO_EXCEPTIONS
					_res = pipe_layout.result;
					VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _res);
					_pipelayout = pipe_layout.value;
#else
					_pipelayout = pipe_layout;
#endif
				}
			}

			template<class... Args>
			auto descriptor_pool_size(Args&... args)-> std::vector<vhn::DescriptorPoolSize> {
				constexpr auto N = sizeof...(args);
				auto arr_mem = std::array<vuh::mem::BasicMemory*, N>{ &args ... };
				std::map<vhn::DescriptorType, size_t> desc_types;
				std::map<vhn::DescriptorType, size_t>::iterator it;
				for(size_t i = 0; i < N ; i ++) {
					it = desc_types.find(arr_mem[i]->descriptorType());
					if(it == desc_types.end()) {
						desc_types.insert(std::make_pair(arr_mem[i]->descriptorType(),1));
					} else {
						it->second = it->second + 1;
					}
				}
				std::vector<vhn::DescriptorPoolSize> desc_pool_size;
				for(it = desc_types.begin(); it != desc_types.end(); it++) {
					vhn::DescriptorType desc_type = it->first;
					uint32_t count = it->second;
					desc_pool_size.push_back({desc_type, count});
				}
				return desc_pool_size;
			}

			/// Allocates descriptors sets
			template<class... Args>
			auto alloc_descriptor_sets(Args&... args)-> void {
				assert(_dsclayout);
				auto descriptor_sizes = descriptor_pool_size(args...);
				auto pool =  _dev.createDescriptorPool(
						{vhn::DescriptorPoolCreateFlags(), 1 // 1 here is the max number of descriptor sets that can be allocated from the pool
								, uint32_t(descriptor_sizes.size()), descriptor_sizes.data()
						});
				_res = vhn::Result::eSuccess;
#ifdef VULKAN_HPP_NO_EXCEPTIONS
				_res = pool.result;
				VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _res);
				_dscpool = pool.value;
#else
				_dscpool = pool;
#endif
				if (bool(_dscpool)) {
					auto desc_set = _dev.allocateDescriptorSets({_dscpool, 1, &_dsclayout});
#ifdef VULKAN_HPP_NO_EXCEPTIONS
					_res = pool.result;
					VULKAN_HPP_ASSERT(vhn::Result::eSuccess == _res);
					_dscset = desc_set.value[0];
#else
					_dscset = desc_set[0];
#endif
				}
			}

			/// Starts writing to the device's compute command buffer.
			/// Binds a pipeline and a descriptor set.
			template<class... Args>
			auto command_buffer_begin(Args&... args)-> void {
				assert(_pipeline); /// pipeline supposed to be initialized before this

				constexpr auto N = sizeof...(args);
				auto arr_mem = std::array<vuh::mem::BasicMemory*, N>{ &args ... };
				auto write_dscsets = dscinfos2writesets(_dscset, arr_mem
				                                       , std::make_index_sequence<N>{});
				_dev.updateDescriptorSets(write_dscsets, {}); // associate buffers to binding points in bindLayout

				// Start recording commands into the newly allocated command buffer.
				//	auto beginInfo = vhn::CommandBufferBeginInfo(vhn::CommandBufferUsageFlagBits::eOneTimeSubmit); // buffer is only submitted and used once
				auto cmdbuf = _dev.computeCmdBuffer();
				auto beginInfo = vhn::CommandBufferBeginInfo();
				cmdbuf.begin(beginInfo);

				// Before dispatch bind a pipeline, AND a descriptor set.
				cmdbuf.bindPipeline(vhn::PipelineBindPoint::eCompute, _pipeline);
				cmdbuf.bindDescriptorSets(vhn::PipelineBindPoint::eCompute, _pipelayout
				                          , 0, {_dscset}, {});
			}

			/// Ends command buffer creation. Writes dispatch info and signals end of commands recording.
			auto command_buffer_end()-> void {
				auto cmdbuf = _dev.computeCmdBuffer();
				cmdbuf.dispatch(_batch[0], _batch[1], _batch[2]); // start compute pipeline, execute the shader
				cmdbuf.end(); // end recording commands
			}
		protected: // data
			vhn::ShaderModule _shader;            ///< compute shader to execute
			vhn::DescriptorSetLayout _dsclayout;  ///< descriptor set layout. This defines the kernel's array parameters interface.
			vhn::DescriptorPool _dscpool;         ///< descitptor ses pool. Descriptors are allocated on this pool.
			vhn::DescriptorSet _dscset;           ///< descriptors set
			vhn::PipelineCache _pipecache;        ///< pipeline cache
			vhn::PipelineLayout _pipelayout;      ///< pipeline layout
			mutable vhn::Pipeline _pipeline;      ///< pipeline itself

			vuh::Device& _dev;                ///< refer to device to run shader on
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
			SpecsBase(Device& dev, const char* filepath, vhn::ShaderModuleCreateFlags flags={})
			   : ProgramBase(dev, filepath, flags)
			{}

			/// Construct object using given a vuh::Device a SPIR-V shader code.
			SpecsBase(Device& dev, const std::vector<char>& code, vhn::ShaderModuleCreateFlags f={})
			   : ProgramBase(dev, code, f)
			{}

			/// Initialize the pipeline.
			/// Specialization constants interface is defined here.
			auto init_pipeline()-> void {
				auto specEntries = specs2mapentries(_specs);
				auto specInfo = vhn::SpecializationInfo(uint32_t(specEntries.size()), specEntries.data()
																	, sizeof(_specs), &_specs);

				// Specify the compute shader stage, and it's entry point (main), and specializations
				auto stageCI = vhn::PipelineShaderStageCreateInfo(vhn::PipelineShaderStageCreateFlags()
																				 , vhn::ShaderStageFlagBits::eCompute
																				 , _shader, "main", &specInfo);
				_pipeline = _dev.createPipeline(_pipelayout, _pipecache, stageCI, _res);
			}
		protected:
			std::tuple<Spec_Ts...> _specs; ///< hold the state of specialization constants between call to specs() and actual pipeline creation
		};

		/// Explicit specialization for empty specialization constants interface.
		template<>
		class SpecsBase<typelist<>>: public ProgramBase {
		protected:
			/// Construct object using given a vuh::Device and path to SPIR-V shader code.
			SpecsBase(Device& dev, const char* filepath, vhn::ShaderModuleCreateFlags flags={})
			   : ProgramBase(dev, filepath, flags)
			{}

			/// Construct object using given a vuh::Device a SPIR-V shader code.
			SpecsBase(Device& dev, const std::vector<char>& code, vhn::ShaderModuleCreateFlags f={})
			   : ProgramBase(dev, code, f)
			{}

			/// Initialize the pipeline with empty specialialization constants interface.
			auto init_pipeline()-> void {
				auto stageCI = vhn::PipelineShaderStageCreateInfo(vhn::PipelineShaderStageCreateFlags()
																				 , vhn::ShaderStageFlagBits::eCompute
																				 , _shader, "main", nullptr);

				_pipeline = _dev.createPipeline(_pipelayout, _pipecache, stageCI, _res);
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
		Program(vuh::Device& dev, const char* filepath, vhn::ShaderModuleCreateFlags flags={})
		   : Base(dev, read_spirv(filepath), flags)
		{}

		/// Initialize program on a device from binary SPIR-V code
		Program(vuh::Device& dev, const std::vector<char>& code
		        , vhn::ShaderModuleCreateFlags flags={}
		        )
		   : Base(dev, code, flags)
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

		/// Initiate execution of the program with provided parameters and immidiately return.
		/// if suspend == true we will add an event on the top of command queue ,the command will wait until event is fired
		/// we use this for precise time measurement or command pool
		/// we submit some tasks to driver and wake up them when we need
		/// it's not thread safe , please call resume on the thread who create the program
		/// do'nt wait for too long time ,as we know timeout may occur about 2-3 seconds later
		/// @return Delayed<Compute> object for synchronization with host.
		/// @pre grid dimensions should be specified before callind this.
		template<class... Arrs>
		auto run_async(const Params& params,bool suspend, Arrs&&... args)-> vuh::Delayed<detail::Compute> {
			bind(params, args...);
			return Base::run_async(suspend);
		}
	private: // helpers
		/// Set up the state of the kernel that depends on number and types of bound array parameters.
		/// Initizalizes the pipeline layout, declares the push constants interface.
		template<class... Arrs>
		auto init_pipelayout(Arrs&... args)-> void {
			auto psranges = std::array<vhn::PushConstantRange, 1>{{vhn::PushConstantRange(vhn::ShaderStageFlagBits::eCompute, 0, sizeof(Params))}};
			Base::init_pipelayout(psranges, args...);
		}

		/// Populate the associated device's compute command buffer.
		/// Binds the descriptors and pushes the push constants.
		template<class... Arrs>
		auto create_command_buffer(const Params& p, Arrs&... args)-> void {
			Base::command_buffer_begin(args...);
			Base::_dev.computeCmdBuffer().pushConstants(Base::_pipelayout
			                           , vhn::ShaderStageFlagBits::eCompute , 0, sizeof(p), &p);
			Base::command_buffer_end();
		}
	}; // class Program

	/// Specialization with non-empty specialization constants and empty push constants.
	template<template<class...> class Specs, class... Specs_Ts>
	class Program<Specs<Specs_Ts...>, typelist<>>: public detail::SpecsBase<Specs<Specs_Ts...>>{
		using Base = detail::SpecsBase<Specs<Specs_Ts...>>;
	public:
		/// Initialize program on a device using SPIR-V code at a given path
		Program(vuh::Device& dev, const char* filepath, vhn::ShaderModuleCreateFlags flags={})
		   : Base(dev, filepath, flags)
		{}

		/// Initialize program on a device from binary SPIR-V code
		Program(vuh::Device& dev, const std::vector<char>& code
		        , vhn::ShaderModuleCreateFlags flags={}
		        )
		   : Base (dev, code, flags)
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
				Base::init_pipelayout(std::array<vhn::PushConstantRange, 0>{}, args...);
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
