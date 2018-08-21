#pragma once

#include "arrayIter.hpp"
#include "deviceArray.hpp"
#include <vuh/delayed.hpp>
#include <vuh/traits.hpp>
#include <vuh/resource.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace vuh {
	namespace detail {
		/// Command buffer data packed with allocation and deallocation methods.
		struct _CmdBuffer {
			/// Constructor. Creates the new command buffer on a provided device and manages its resources.
			_CmdBuffer(vuh::Device& device): device(&device){
				auto bufferAI = vk::CommandBufferAllocateInfo(device.transferCmdPool()
																			 , vk::CommandBufferLevel::ePrimary, 1);
				cmd_buffer = device.allocateCommandBuffers(bufferAI)[0];
			}

			/// Constructor. Takes ownership over the provided buffer.
			/// @pre buffer should belong to the provided device. No check is made even in a debug build.
			_CmdBuffer(vuh::Device& device, vk::CommandBuffer buffer)
				: cmd_buffer(buffer), device(&device)
			{}

			/// Release the buffer resources
			auto release() noexcept-> void {
				if(device){
					device->freeCommandBuffers(device->transferCmdPool(), 1, &cmd_buffer);
				}
			}
		public: // data
			vk::CommandBuffer cmd_buffer; ///< command buffer managed by this wrapper class
			std::unique_ptr<vuh::Device, util::NoopDeleter<vuh::Device>> device; ///< device holding the buffer
		}; // struct _CmdBuffer

		/// Movable command buffer class.
		using CmdBuffer = util::Resource<_CmdBuffer>;

		/// Implements the actual async copy between two device buffers.
		/// Owns the transient transfer command buffer.
		/// Used to keep that alive till async copy is over.
		/// The delayed action associated with operator() is a noop.
		struct CopyDevice: private CmdBuffer {
			CopyDevice(vuh::Device& device): CmdBuffer(device){}

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
		}; // struct CopyDevice

		/// Keeps the staging array and transfer command buffer alive till async copy completes.
		/// Delayed action is a noop.
		/// At construction copies the data from host to the staging buffer.
		template<class T>
		struct CopyStageFromHost: public CopyDevice {
			using StageArray = arr::HostArray<T, arr::AllocDevice<arr::properties::HostCoherent>>;
			StageArray array; ///< staging buffer

			/// Constructor. Copies data from host to the internal staging buffer.
			template<class Iter1, class Iter2>
			CopyStageFromHost(vuh::Device& device, Iter1 src_begin, Iter2 src_end)
				: CopyDevice(device), array(device, src_begin, src_end)
			{}
		}; // struct CopyStageFromHost

		/// Keeps the staging buffer and the transfer command buffer alive till async copy completes.
		/// Delayed action copies data from staging buffer to the host.
		template<class T, class IterDst>
		struct CopyStageToHost: CopyDevice {
			using StageArray = arr::HostArray<T, arr::AllocDevice<arr::properties::HostCached>>;
			StageArray array;      ///< staging buffer
			IterDst    dst_begin;  ///< iterator to beginning of the host destination range

			/// Constructor.
			explicit CopyStageToHost(vuh::Device& device, std::size_t array_size, IterDst dst_begin)
			   : CopyDevice(device), array(device, array_size), dst_begin(dst_begin)
			{}

			/// Delayed action. Copies data from staging buffer to the host.
			auto operator()() const-> void {
				using std::begin; using std::end;
				std::copy(begin(array), end(array), dst_begin);
			}
		}; // struct StagedCopy

		/// Delayed action copies data from host-visible device buffer to host.
		/// Buffer is expected to exist till the copy is complete.
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
	/// Used to trigger some action (encoded in that operator()()) and/or extend resources
	/// lifetime at/till the synchrnonization point.
	class Copy {
	public:
		/// Takes an object of some type T, creates a CopyWrapper<T> of it on the heap
		/// and creates an object of Copy class on top of that.
		template<class T> 
		static auto wrap(T&& t)-> Copy {
			return Copy(std::make_unique<detail::CopyWrapper<T>>(std::move(t)));
		}

		/// Runs the operator() of underlying type-erased object.
		/// Presumed semantics of operator() is the delayed data copy operation.
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
	/// Blocks while for the duration of initial copy from host memory to host-visible
	/// staging array.
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
			return Delayed<Copy>{array.device(), Copy::wrap(detail::Noop{})};
		} else { // copy first to staging buffer and then async copy from staging buffer to device
			auto stage = detail::CopyStageFromHost<T>(array.device(), src_begin, src_end);
			return Delayed<Copy>{
				         stage.copy_async(device_begin(stage.array), device_end(stage.array), dst_begin)
			          , Copy::wrap(std::move(stage))
			};
		}
	}

	/// Async copy data from device-local array to host.
	/// Initiates async copy from device to the staging buffer and immidiately returns
	/// the Delayed<Copy>  object used for synchronization with host.
	/// The copy between staging buffer and host is only triggered at the synchronization point
	/// (Delayed<Copy>::wait() or destructor) and it blocks till the complete operation is finished.
	/// If device array is host-visible it just makes the blocking call to std::copy().
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
			return Delayed<Copy>{ array.device()
			                    , Copy::wrap(detail::StdCopy<SrcIter, DstIter>(src_begin, src_end, dst_begin))};
		}
	}
} // namespace vuh
