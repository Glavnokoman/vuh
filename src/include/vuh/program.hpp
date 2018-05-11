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
	
		template<class T> struct DictTypeToDsc<vuh::Array<T>>{
			static constexpr vk::DescriptorType value = vk::DescriptorType::eStorageBuffer;
		};
		
		// Compile-time offset of tuple element.
		// Probably UB as tuples are non-POD types (no easy way out till C++ gets some compile-time reflection).
		// Should never be called outside constexpr context.
		template<size_t Idx, class T>
		constexpr auto tuple_element_offset(const T& tup)-> std::size_t {
			return size_t(reinterpret_cast<const char*>(&std::get<Idx>(tup))
		                 - reinterpret_cast<const char*>(&tup));
		}
		
		//
		template<class T, size_t... I>
		constexpr auto spec2entries(const T& specs, std::index_sequence<I...>){
			return std::array<vk::SpecializationMapEntry, sizeof...(I)>{{
			                       { uint32_t(I)
			                       , uint32_t(tuple_element_offset<I>(specs))
			                       , uint32_t(sizeof(typename std::tuple_element<I, T>::type))
			                       }...
			}};
		}
	} // namespace trais

	/// doc me
	template<class... Ts>
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
	template<class... Ts>
	auto specs2mapentries(const std::tuple<Ts...>& specs
	                      )-> std::array<vk::SpecializationMapEntry, sizeof...(Ts)> 
	{
		return detail::spec2entries(specs, std::make_index_sequence<sizeof...(Ts)>{});
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
} // namespace

namespace vuh {
	template<class... Ts>	 struct typelist{};

	template<class T> class Program;

	namespace detail {
		/// doc me
		template<class Params, class... Specs>
		class ProgramBindable {
			using parent_t = Program<Params>;
			std::tuple<Specs...> _specs;
			parent_t& _p;
		public:
			explicit ProgramBindable(parent_t& program, Specs... specs)
				: _specs(specs...), _p(program)
			{}

			template<class... Ts>
			auto bind(const Params& p, Ts&... arrays)-> parent_t& {
				_p.init_pipelayout(arrays...);
				init_pipeline();
				_p.create_command_buffer(p, arrays...);
				return _p;
			}

			template<class... Ts>
			auto operator()(const Params& p, Ts&... arrays)-> void {
				bind(p, arrays...);
				_p.run();
			}
		private: // helpers
			auto init_pipeline()-> void {
				auto specEntries = specs2mapentries(_specs);
				auto specInfo = vk::SpecializationInfo(uint32_t(specEntries.size()), specEntries.data()
				                                       , sizeof(_specs), &_specs);

				// Specify the compute shader stage, and it's entry point (main), and specializations
				auto stageCI = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags()
				                                                 , vk::ShaderStageFlagBits::eCompute
				                                                 , _p._shader, "main", &specInfo);
				_p._pipeline = _p._device.createPipeline(_p._pipelayout, _p._pipecache, stageCI);
			}
		}; // class Program_
	} // namespace detail

	/// specialization to unpack array types parameters
	template<class Params> ///< shader push parameters structure
	class Program {
		template<class P, class... Specs> friend class detail::ProgramBindable;
	public:
		Program(vuh::Device& device, const std::vector<char>& code
		        , vk::ShaderModuleCreateFlags flags
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
		auto grid(uint32_t x, uint32_t y = 1, uint32_t z = 1)-> Program& {
			_batch = {x, y, z};
			return *this;
		}
		
		/// Specify values of specification constants.
		template<class... Us>
		auto spec(Us... specs)-> detail::ProgramBindable<Params, Us...> {
			// specialize constants of the shader
			return detail::ProgramBindable<Params, Us...>(*this, specs...);
		}
		
		/// Associate buffers to binding points, and pushes the push constants.
		/// Does most of setup here. Programs is ready to be run.
		/// @pre Specs and batch sizes should be specified before calling this.
		template<class... Ts>
		auto bind(const Params& p, Ts&... args)-> const Program& {
			init_pipelayout<Ts...>();
			init_pipeline();
			create_command_buffer(p, args...);
			return *this;
		}

		/// Release device resources allocated by parameters binding.
		/// @todo hide it or remove, should never be explicitely called.
		auto unbind()-> void {
			throw "not implemented"; // what follows is not a proper implementation. left here for reference.
			_device.destroyDescriptorPool(_dscpool);
			_device.resetCommandPool(_device.computeCmdBuffer(), vk::CommandPoolResetFlags());
			auto sbo_descriptors_size = vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer
			                                                   , _num_sbo_params);
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

		template<class... Ts>
		auto operator()(const Params& params, Ts&... args)-> void {
			bind(params, args...);
			run();
		}
	private: // helpers
		/// set up the state of the kernel that depends on number and types of bound array parameters
		template<class... Ts ///< bound array-like parameters
		         >
		auto init_pipelayout(Ts&...)-> void {
			_num_sbo_params = sizeof...(Ts);
			auto dscTypes = typesToDscTypes<Ts...>();
			auto sbo_descriptors_size = vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer
			                                                   , _num_sbo_params);
			auto descriptor_sizes = std::vector<vk::DescriptorPoolSize>({sbo_descriptors_size}); // can be done compile-time, but not worth it
			_dscpool = _device.createDescriptorPool(
			                        {vk::DescriptorPoolCreateFlags(), 1 // 1 here is the max number of descriptor sets that can be allocated from the pool
			                         , uint32_t(descriptor_sizes.size()), descriptor_sizes.data()
			                         }
			);

			auto bindings = dscTypesToLayout(dscTypes);
			_dsclayout = _device.createDescriptorSetLayout(
			                                             {vk::DescriptorSetLayoutCreateFlags()
			                                              , uint32_t(bindings.size()), bindings.data()
			                                              }
			);
			_dscset = _device.allocateDescriptorSets({_dscpool, 1, &_dsclayout})[0];
			_pipecache = _device.createPipelineCache({});
			auto push_constant_range = vk::PushConstantRange(vk::ShaderStageFlagBits::eCompute
			                                                 , 0, sizeof(Params));
			_pipelayout = _device.createPipelineLayout(
						     {vk::PipelineLayoutCreateFlags(), 1, &_dsclayout, 1, &push_constant_range});
		}

		///
		auto init_pipeline()-> void {
			// Specify the compute shader stage, and it's entry point (main), and specializations
			auto stageCI = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags()
			                                                 , vk::ShaderStageFlagBits::eCompute
			                                                 , _shader, "main", nullptr);

			_pipeline = _device.createPipeline(_pipelayout, _pipecache, stageCI);
		}

		///
		template<class... Ts>
		auto create_command_buffer(const Params& p, Ts&... args)-> void {
			constexpr auto N = sizeof...(args);
			auto dscinfos = std::array<vk::DescriptorBufferInfo, N>{{{args, 0, args.size_bytes()}... }}; // 0 is the offset here
			auto write_dscsets = dscinfos2writesets(_dscset, dscinfos, std::make_index_sequence<N>{});
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
		vk::DescriptorPool _dscpool;
		vk::DescriptorSetLayout _dsclayout;
		vk::DescriptorSet _dscset;
		vk::PipelineCache _pipecache;
		vk::PipelineLayout _pipelayout;
		mutable vk::Pipeline _pipeline;

		std::array<uint32_t, 3> _batch={0, 0, 0};
		std::size_t _num_sbo_params;

		vuh::Device& _device;
	}; // class Program
} // namespace vuh
