#pragma once

#include <type_traits>
#include <limits>
#include <cmath>
#include <iostream>

namespace test{

using std::begin; using std::end;

namespace traits {
	namespace detail {
		template<class T>
		auto is_iterable_impl(int)
			-> decltype( begin(std::declval<T&>()) != end(std::declval<T&>())    // begin/end and operator !=
							, void()                                                 // Handle evil operator ,
							, ++std::declval<decltype(begin(std::declval<T&>()))&>() // operator ++
							, void(*begin(std::declval<T&>()))                       // operator*
							, std::true_type{}
		               );
	
		template<class T> auto is_iterable_impl(...)-> std::false_type;
	} // namespace detail

	template<class T> using is_iterable = decltype(detail::is_iterable_impl<T>(0));
} // namespace traits

	namespace detail {
		template<class T>
		auto is_close(const T& t1, const T& t2, const T& eps)-> bool {
			auto ret = std::abs(2.0*(t1 - t2)) <= std::abs(eps*(t1 + t2)); //eps*std::max(std::abs(t1), std::abs(t2)));
			return ret;
		}
	
		template<class T>
		struct ApproxIterable {
			using value_t = std::decay_t<decltype(*begin(std::declval<T>()))>;
			
			explicit ApproxIterable(const T& values
			                       , value_t eps=std::numeric_limits<value_t>::epsilon()*100
			                       )
				: _values{values}, _eps{eps}
			{}
			
			auto eps(double e)-> ApproxIterable& {  _eps = e; return *this; }
	
			auto verbose()-> ApproxIterable& { _verbose = true; return *this; }
	
			template<class U, typename std::enable_if_t<traits::is_iterable<T>::value>* = nullptr>
			friend auto operator== (const U& v1, const ApproxIterable& app)-> bool {
				if(v1.size() != app._values.size()){ // size() strictly speaking not required by iterable trait
					if(app._verbose){
						std::cerr << "approximate comparison failed: different iterables size" << "\n";
					}
					return false;
				}
	
				auto it_v1 = begin(v1);
				auto it_v2 = begin(app._values);
				for(size_t i = 0; it_v1 != end(v1); ++it_v1, ++it_v2, ++i){
					if(!is_close(*it_v1, *it_v2, app._eps)){
						if(app._verbose){
							std::cerr << "approximate compare failed at offset " << i
										 << ": " << *it_v1 << " ~= " << *it_v2 << std::endl;
						}
						return false;
					}
				}
				if(it_v2 != end(app._values)){
					if(app._verbose){
						std::cerr << "approximate compare failed: different iterables size" << std::endl;
					}
					return false;
				}
				return true;
			}
	
			friend auto operator<< (std::ostream& out, const ApproxIterable& app)-> std::ostream& {
				if(app._values.size() < 16) {
					for(const auto& v: app._values){
						out << v << ",";
					}
				} else {
					out << app._values.front() << "\n...\n" << app._values.back();
				}
				return out;
			}
		private:
			const T& _values;
			value_t _eps;
			bool _verbose = false;
		}; // struct ApproxIterable
	
		template<class T>
		struct ApproxScalar {
			explicit ApproxScalar(const T& value
			                      , T eps=std::numeric_limits<T>::epsilon()*100
			                     )
			   : _val{value}, _eps{eps}
			{}
			
			auto eps(double e)-> ApproxScalar& { _eps = e; return *this; }
	
			template<class U>
			friend auto operator== (const U& v1, const ApproxScalar& app)-> bool {
				return is_close(v1, app._val, app._eps);
			}
	
			friend auto operator<< (std::ostream& out, const ApproxScalar& app)-> std::ostream& {
				out << app._val;
				return out;
			}
		private:
			const T& _val;
			double _eps;
		}; // struct ApproxScalar
	
	} // namespace detail

	/// factory function for iterable approximators
	template<class T>
	auto approx(const T& values
	            , typename std::enable_if_t<traits::is_iterable<T>::value>* = nullptr
	           )
	{
		return detail::ApproxIterable<T>(values);
	}
	
	/// factory function for scalar approximators
	template<class T>
	auto approx(const T& val
	            , typename std::enable_if_t<!traits::is_iterable<T>::value>* = nullptr
	           )
	{
		return detail::ApproxScalar<T>(val);
	}

} //namespace test
