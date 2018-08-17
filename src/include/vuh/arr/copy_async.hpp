#pragma once

#include "arrayIter.hpp"
#include <vuh/delayed.hpp>
#include <vuh/traits.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace vuh {
	namespace detail {
		/// RAII wraper around transfer command buffer to use for async copy operation.
		/// Satisfies Copy strategy with noop operator() for delayed action.
		struct CopyDevice {
			CopyDevice(const CopyDevice&) = delete;
			auto operator= (const CopyDevice&)-> CopyDevice& = delete;
			CopyDevice(CopyDevice&&) = default;
			auto operator= (CopyDevice&&)-> CopyDevice& = default;
			///
			CopyDevice(vuh::Device& device): device(&device){
				auto bufferAI = vk::CommandBufferAllocateInfo(device.transferCmdPool()
				                                              , vk::CommandBufferLevel::ePrimary, 1);
				cmd_buffer = device.allocateCommandBuffers(bufferAI)[0];
			}

			~CopyDevice(){
				if(device){
					device->freeCommandBuffers(device->transferCmdPool(), 1, &cmd_buffer);
				}
			}

			/// delayed operation is a noop
			constexpr auto operator()() const-> void {}

			template<class Array1, class Array2>
			auto copy_async(ArrayIter<Array1> src_begin, ArrayIter<Array1> src_end
			                , ArrayIter<Array2> dst_begin
			                )-> Delayed<>
			{
				assert(device);
				assert(*device == dst_begin.array().device());
				using value_type_src = typename ArrayIter<Array1>::value_type;
				using value_type_dst = typename ArrayIter<Array2>::value_type;
				static_assert(std::is_same<value_type_src, value_type_dst>::value
				              , "array value types should be the same");
				static constexpr auto tsize = sizeof(value_type_src);

				cmd_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
				auto region = vk::BufferCopy(tsize*src_begin.offset(), tsize*dst_begin.offset()
				                            , tsize*(src_end - src_begin));
				cmd_buffer.copyBuffer(src_begin.array(), dst_begin.array(), 1, &region);
				cmd_buffer.end();

				auto queue = device->transferQueue();
				auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buffer);
				auto fence = device->createFence(vk::FenceCreateInfo());
				queue.submit({submit_info}, fence);

				return Delayed<>{fence, *device};
			}

		private: // data
			struct _noop { constexpr auto operator()(vuh::Device*) noexcept-> void {} };
			vk::CommandBuffer cmd_buffer;
			std::unique_ptr<vuh::Device, _noop> device;
		}; // struct CopyDevice

		///
		template<class T>
		struct CopyStageFromHost: public CopyDevice {
			using StageArray = arr::HostArray<T, arr::AllocDevice<arr::properties::HostCoherent>>;
			StageArray array; ///< staging buffer

			template<class Iter1, class Iter2>
			CopyStageFromHost(vuh::Device& device, Iter1 src_begin, Iter2 src_end)
				: CopyDevice(device), array(device, src_begin, src_end)
			{}
		}; // struct CopyStageFromHost

		/// doc me
		template<class T, class IterDst>
		struct CopyStageToHost: CopyDevice {
			using StageArray = arr::HostArray<T, arr::AllocDevice<arr::properties::HostCached>>;

			StageArray array;      ///< staging buffer
			IterDst    dst_begin;  ///< iterator to beginning of the host destination range
			explicit CopyStageToHost(vuh::Device& device, std::size_t array_size, IterDst dst_begin)
			   : CopyDevice(device), array(device, array_size), dst_begin(dst_begin)
			{}

			auto operator()() const-> void {
				using std::begin; using std::end;
				std::copy(begin(array), end(array), dst_begin);
			}
		}; // struct StagedCopy

		/// doc me
		template<class IterSrc, class IterDst>
		struct StdCopy {
			IterSrc src_begin, src_end;
			IterDst dst_begin;

			StdCopy(IterSrc src_begin, IterSrc src_end, IterDst dst_begin)
			   : src_begin(src_begin), src_end(src_end), dst_begin(dst_begin)
			{}

			auto operator()() const-> void {
				const auto& array = src_begin.array();
				array.rangeToHost(src_begin.offset(), src_end.offset(), dst_begin);
			}
		}; // struct StdCopy

		/// Virtual interface for callables with operator()(void) const-> void.
		class ICopy{
		public:
			virtual auto operator()() const-> void = 0;
			virtual ~ICopy() = default;
		};

		/// Wraps movable classes with non-virtual operator()(void) const-> void interface to
		/// a class with ICopy virtual interface.
		template<class T>
		class CopyWrapper: public ICopy, private T {
		public:
			CopyWrapper(T&& t): T(std::move(t)) {}
			auto operator()() const-> void override { return T::operator()();}
			~CopyWrapper() override = default;
		};
	} // namespace detail

	/// Type erasure over movable classes providing operator()(void) const-> void.
	class Copy {
	public:
		/// Takes an object of some type T, creates a CopyWrapper<T> of it on the heap
		/// and creates an object of Copy class on top of that.
		template<class T> 
		static auto wrap(T&& t)-> Copy {
			return Copy(std::make_unique<detail::CopyWrapper<T>>(std::move(t)));
		}

		/// runs the operator() of underlying type-erased object
		auto operator()() const-> void {
			assert(_obj);
			(*_obj)();
		}
	private:
		explicit Copy(std::unique_ptr<detail::ICopy>&& ptr): _obj(std::move(ptr)) {}
	private:
		std::unique_ptr<detail::ICopy> _obj; ///< doc me
	};

	/// Async copy between arrays allocated on the same device
	template<class Array1, class Array2>
	auto copy_async(ArrayIter<Array1> src_begin, ArrayIter<Array1> src_end
	                , ArrayIter<Array2> dst_begin
	                )-> vuh::Delayed<Copy>
	{
		auto& src_device = src_begin.array().device();
		auto copyDevice = detail::CopyDevice(src_device);
		return Delayed<Copy>{copyDevice.copy_async(src_begin, src_end, dst_begin)
		                    , Copy::wrap(std::move(copyDevice))};
	}

	/// Async copy data from host memory to device-local array.
	/// Blocks while copying from host memory to host-visible staging array.
	/// Only the memory transfer between staging buffer and device memory is actually async.
	/// If device array is host-visible the operation is fully blocking.
	template<class SrcIter1, class SrcIter2, class T, class Alloc>
	auto copy_async(SrcIter1 src_begin, SrcIter2 src_end
	           , vuh::ArrayIter<arr::DeviceArray<T, Alloc>> dst_begin
	           )-> std::enable_if_t<traits::are_comparable_host_iterators<SrcIter1, SrcIter2>::value
	                               , vuh::Delayed<Copy>
	                               >
	{
		auto& array = dst_begin.array();
		if(array.isHostVisible()){ // normal copy, the function blocks till the copying is complete
			array.fromHost(src_begin, src_end, dst_begin.offset());
			return Delayed<Copy>{Fence(), Copy::wrap(detail::Noop{})};
		} else { // copy first to staging buffer and then async copy from staging buffer to device
			auto stage = detail::CopyStageFromHost<T>(array.device(), src_begin, src_end);
			return Delayed<Copy>{
				         stage.copy_async(device_begin(stage.array), device_end(stage.array), dst_begin)
			          , Copy::wrap(std::move(stage))
			};
		}
	}

	/// Async copy data from device-local array to host.
	template<class T, class Alloc, class DstIter>
	auto copy_async(ArrayIter<arr::DeviceArray<T, Alloc>> src_begin
	               , ArrayIter<arr::DeviceArray<T, Alloc>> src_end
	               , DstIter dst_begin
	               )-> std::enable_if_t<traits::is_host_iterator<DstIter>::value
	                                   , vuh::Delayed<Copy>
	                                   >
	{
		auto& array = src_begin.array();
		if(!array.isHostVisible()){ // device array is not host-visible
			auto stage = detail::CopyStageToHost<T, DstIter>(array.device(), src_end - src_begin, dst_begin);
			return Delayed<Copy>{ stage.copy_async(src_begin, src_end, device_begin(stage.array))
			                    , Copy::wrap(std::move(stage))};
		} else { // array is host visible
			using SrcIter = ArrayIter<arr::DeviceArray<T, Alloc>>;
			return Delayed<Copy>{ Fence{}
			                    , Copy::wrap(detail::StdCopy<SrcIter, DstIter>(src_begin, src_end, dst_begin))};
		}
	}
} // namespace vuh
