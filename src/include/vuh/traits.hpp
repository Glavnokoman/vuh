#pragma once

#include <type_traits>

namespace vuh {
namespace traits {
	namespace detail {
		/// 
		template<class T>
		auto is_iterable_impl(int)
		   -> decltype( begin(std::declval<T&>()) != end(std::declval<T&>())    // comparable begin/end
		               , void()                                                 // handle evil operator ,
		               , ++std::declval<decltype(begin(std::declval<T&>()))&>() // operator++
		               , void(*begin(std::declval<T&>()))                       // operator*
		               , std::true_type{}
		               );
		template<class T> auto is_iterable_impl(...)-> std::false_type;
		
		///
		template<class T>		
		auto is_contiguous_iterable_impl(int)
		   -> decltype( T::data() + T::size()
		               , void()
		               , void(*T::data())
		               , std::true_type{});

		///
		template<class... T> auto _are_comparable_host_iterators(...)-> std::false_type;

		///
		template<class T1, class T2>
		auto _are_comparable_host_iterators(int)
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
	} // namespace detail
	
	/// Concept to check if given type is iterable
	/// (provides begin(), end(), op++(), and * operators).
	template<class T> using is_iterable = decltype(detail::is_iterable_impl<T>(0));
	
	/// Concept to check if given container is contiguously iterable
	/// (provides data(), size() and * operator)
	template<class T> using is_contiguous_iterable = decltype(detail::is_contiguous_iterable_impl<T>(0));

	/// doc me
	template<class T1, class T2>
	using are_comparable_host_iterators = decltype(detail::_are_comparable_host_iterators<T1, T2>(0));

	/// doc me
	template<class T> using is_host_iterator = decltype(detail::_is_host_iterator<T>(0));
} // namespace traits
} // namespace vuh
