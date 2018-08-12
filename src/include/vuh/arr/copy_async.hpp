#pragma once

#include "arrayIter.hpp"
#include <vuh/fence.hpp>

#include <type_traits>
#include <utility>

namespace vuh {
	
	namespace detail {
		template<class... T> auto _are_comparable_host_iterators(...)-> std::false_type;
		
		template<class T1, class T2>
		auto _are_comparable_host_iterators(int)
		   -> decltype( std::declval<T1&>() != std::declval<T2&>() // operator !=
		              , void()                                     // handle evil operator ,
		              , ++std::declval<T1&>()                      // operator ++
		              , void(*std::declval<T1&>())                 // operator*
		              , std::true_type{}
		              );

		template<class T1, class T2> 
		using are_comparable_host_iterators = decltype(_are_comparable_host_iterators<T1, T2>(0));

		template<class... T> auto _is_host_iterator(...)-> std::false_type;
		template<class T>
		auto _is_host_iterator(int)
		   -> decltype( ++std::declval<T&>()      // has operator ++
		              , void()
		              , void(*std::declval<T&>()) // and has operator*
		              , std::true_type{}
		              );

		template<class T> using is_host_iterator = decltype(_is_host_iterator<T>(0));
	} // namespace detail

	///
	struct CopyDevice {
		CopyDevice(const CopyDevice&) = delete;
		auto operator= (const CopyDevice&)-> CopyDevice& = delete;
		CopyDevice(CopyDevice&&) = default;
		auto operator= (CopyDevice&&)-> CopyDevice& = default;

		CopyDevice(vuh::Device& device);
		~CopyDevice(){
			device.freeCommandBuffers(device.computeCmdPool(), 1, &cmd_buffer);
		}

		vk::CommandBuffer cmd_buffer;
		vuh::Device& device;
	}; // struct CopyDevice

	///
	template<class T>
	struct CopyStageFromHost {
		using StageArray = arr::HostArray<T, arr::AllocDevice<arr::properties::HostCoherent>>;
		StageArray array; ///< staging buffer

		template<class Iter1, class Iter2>
		CopyStageFromHost(vuh::Device& device, Iter1 src_begin, Iter2 src_end)
			: array(device, src_begin, src_end)
		{}
		auto operator()() const-> void {}
	}; // struct CopyStageFromHost

	///
	template<class T, class IterDst>
	struct CopyStageToHost{
		using StageArray = arr::HostArray<T, arr::AllocDevice<arr::properties::HostCached>>;

		StageArray array;      ///< staging buffer
		IterDst    dst_begin;  ///< iterator to beginning of the host destination range
		explicit CopyStageToHost(vuh::Device& device, std::size_t array_size, IterDst dst_begin)
		   : array(device, array_size), dst_begin(dst_begin)
		{}

		auto operator()() const-> void {
			using std::begin; using std::end;
			std::copy(begin(array), end(array), dst_begin);
		}
	}; // struct StagedCopy
	
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

	/// Type erasure over movable classes providing operator()(void) const-> void.
	class Copy {
	public:
		template<class T> 
		static auto wrap(T&& t)-> Copy {
			return Copy(std::make_unique<CopyWrapper<T>>(std::move(t)));
		}

		auto operator()() const-> void {
			assert(_obj);
			(*_obj)();
		}
	private:
		explicit Copy(std::unique_ptr<ICopy>&& ptr): _obj(std::move(ptr)) {}
	private:
		std::unique_ptr<ICopy> _obj;
	};

	/// Async copy between vuh arrays allocated on the same device
	template<class Array1, class Array2>
	auto copy_async(ArrayIter<Array1> src_begin, ArrayIter<Array1> src_end
	                , ArrayIter<Array2> dst_begin
	                )-> vuh::Delayed<>
	{
		using value_type_src = typename ArrayIter<Array1>::value_type;
		using value_type_dst = typename ArrayIter<Array2>::value_type;
		static_assert(std::is_same<value_type_src, value_type_dst>::value
		              , "array value types should be the same");
		static constexpr auto tsize = sizeof(value_type_src);

		auto& src_device = src_begin.array().device();
		assert(src_device == dst_begin.array().device());

		auto cmd_buf = src_device.transferCmdBuffer();
		cmd_buf.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		auto region = vk::BufferCopy(tsize*src_begin.offset(), tsize*dst_begin.offset()
		                            , tsize*(src_end - src_begin));
		cmd_buf.copyBuffer(src_begin.array(), dst_begin.array(), 1, &region);
		cmd_buf.end();

		auto queue = src_device.transferQueue();
		auto submit_info = vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buf);
		auto fence = src_device.createFence(vk::FenceCreateInfo());
		queue.submit({submit_info}, fence);

		return vuh::Fence(fence, src_device);
	}

	/// Async copy data from host memory to device-local array.
	/// Blocks while copying from host memory to host-visible staging array.
	/// Only the memory transfer between staging buffer and device memory is actually async.
	/// If device array is host-visible the operation is fully blocking.
	template<class SrcIter1, class SrcIter2, class T, class Alloc>
	auto copy_async(SrcIter1 src_begin, SrcIter2 src_end
	           , vuh::ArrayIter<arr::DeviceArray<T, Alloc>> dst_begin
	           )-> std::enable_if_t<detail::are_comparable_host_iterators<SrcIter1, SrcIter2>::value
	                               , vuh::Delayed<Copy>
	                               >
	{
		auto& array = dst_begin.array();
		if(array.isHostVisible()){ // normal copy, the function blocks till the copying is complete
			array.fromHost(src_begin, src_end, dst_begin.offset());
			return Delayed<Copy>{Fence(), Copy::wrap(detail::Noop{})};
		} else { // copy first to staging buffer and then async copy from staging buffer to device
			auto stage = CopyStageFromHost<T>(array.device(), src_begin, src_end);
			auto fence = copy_async(device_begin(stage.array), device_end(stage.array), dst_begin);
			return Delayed<Copy>{std::move(fence), Copy::wrap(std::move(stage))};
		}
	}

	/// Async copy data from device-local array to host.
	template<class T, class Alloc, class DstIter>
	auto copy_async(ArrayIter<arr::DeviceArray<T, Alloc>> src_begin
	               , ArrayIter<arr::DeviceArray<T, Alloc>> src_end
	               , DstIter dst_begin
	               )-> std::enable_if_t<detail::is_host_iterator<DstIter>::value
	                                   , vuh::Delayed<Copy>
	                                   >
	{
		auto& array = src_begin.array();
		if(!array.isHostVisible()){ // device array is not host-visible
			auto stage = CopyStageToHost<T, DstIter>(array.device(), src_end - src_begin, dst_begin);
			auto fence = copy_async(src_begin, src_end, device_begin(stage.array));
			return Delayed<Copy>{std::move(fence), Copy::wrap(std::move(stage))};
		} else { // array is host visible
			using SrcIter = ArrayIter<arr::DeviceArray<T, Alloc>>;
			return Delayed<Copy>{ Fence{}
				                , Copy::wrap(StdCopy<SrcIter, DstIter>(src_begin, src_end, dst_begin))};
		}
	}
} // namespace vuh
