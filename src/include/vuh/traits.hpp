#pragma once

#include <type_traits>

namespace vuh::traits {
	namespace detail {
		/// 
		template<class T>
		auto is_host_iterable_(int)
		   -> decltype( begin(std::declval<T&>()) != end(std::declval<T&>())    // comparable begin/end
		               , void()                                                 // handle evil operator ,
		               , ++std::declval<decltype(begin(std::declval<T&>()))&>() // operator++
		               , void(*begin(std::declval<T&>()))                       // operator*
		               , std::true_type{}
		               );
		template<class T> auto is_host_iterable_(...)-> std::false_type;
		
		///
		template<class T>
		auto is_contiguous_iterable_impl(int)
		   -> decltype( T::data() + T::size()
		               , void()
		               , void(*T::data())
		               , std::true_type{});

		///
		template<class... T> auto are_comparable_host_iterators_(...)-> std::false_type;

		///
		template<class T1, class T2>
		auto are_comparable_host_iterators_(int)
		   -> decltype( std::declval<T1&>() != std::declval<T2&>() // operator !=
		              , void()                                     // handle evil operator ,
		              , ++std::declval<T1&>()                      // operator ++
		              , void(*std::declval<T1&>())                 // operator*
		              , std::true_type{}
		              );

		///
		template<class... T> auto _is_host_iterator(...)-> std::false_type;
		template<class T>
		auto _is_host_iterator(int)
		   -> decltype( ++std::declval<T&>()      // has operator ++
		              , void()
		              , void(*std::declval<T&>()) // and has operator*
		              , std::true_type{}
		              );

		///
		template<class T, class=void> struct _is_device_buffer: public std::false_type {};
		template<class T> struct _is_device_buffer<T, decltype(
		      (void)(std::declval<T&>().descriptor_class)
		      , void()
		      , (void)(std::declval<T&>().size_bytes() + std::declval<T&>().offset_bytes())
		      )>: public std::true_type {};
	} // namespace detail
	
	/// Concept to check if given type is host-iterable
	/// (provides begin(), end(), op++(), and * operators).
	///
	template<class T> using is_iterable = decltype(detail::is_host_iterable_<T>(0));
	template<class T> inline constexpr bool is_iterable_v = is_iterable<T>::value;
	template<class T> using Iterable = std::enable_if_t<is_iterable_v<T>, T>;
	
	///
	template<class T> using is_device_buffer = detail::_is_device_buffer<T>;
	template<class T> inline constexpr bool is_device_buffer_v = is_device_buffer<T>::value;
	template<class T, class=std::enable_if_t<is_device_buffer_v<T>>> using DeviceBuffer = T;

	/// Concept to check if given container is contiguously iterable
	/// (provides data(), size() and * operator)
	template<class T> using is_contiguous_iterable = decltype(detail::is_contiguous_iterable_impl<T>(0));

	/// doc me
	template<class T1, class T2>
	using are_comparable_host_iterators = decltype(detail::are_comparable_host_iterators_<T1, T2>(0));

	/// doc me
	template<class T> using is_host_iterator = decltype(detail::_is_host_iterator<T>(0));
} // namespace vuh::traits
