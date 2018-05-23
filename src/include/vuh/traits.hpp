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
	} // namespace detail
	
	/// concept to check if given type provides begin(), end(), op++(), and * operators
	template<class T> using is_iterable = decltype(detail::is_iterable_impl<T>(0));
	
	///
	template<class T> using is_contiguous_iterable = decltype(detail::is_contiguous_iterable_impl<T>(0));
} // namespace traits
} // namespace vuh
