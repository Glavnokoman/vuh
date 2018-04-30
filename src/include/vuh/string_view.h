#pragma once

#include <string>
#include <cstring>

namespace vuh {
	#if __cplusplus >= 201703L
		using string_view = std::string_view;
	#else
		namespace detail {
			/// Poor-man c++17 string_view replacement (partial).
			class string_view {
			public:
				using value_type = char;
				using pointer = char*;
				using const_pointer = const char*;

				string_view(const std::string& in)
				   : _begin(in.data()), _end(_begin + in.size()) {}

				string_view(const char* in, size_t size)
				   : _begin(in), _end(in + size) {}

				string_view(const char* in)
				   : _begin(in), _end(in + std::strlen(in)) {}

				template<size_t N> string_view(const char in[N])
				   : _begin(in), _end(in + N) {}


				auto operator[](size_t i) const-> char { return *(_begin + i);}
				auto data() const-> const_pointer { return _begin; }
				auto size() const-> size_t {return size_t(std::distance(_begin, _end));}
				auto empty() const-> bool { return _begin == _end;}

				auto begin() const-> const_pointer { return _begin; }
				auto cbegin() const-> const_pointer { return _begin; }
				auto end() const-> const_pointer { return _end; }
				auto cend() const-> const_pointer { return _end; }

				template<class U>
				friend auto operator==(string_view s, const U& other){
					using std::begin;
					return 0 == std::strncmp(s._begin, begin(other), s.size());
				}

				friend auto operator== (string_view s, const char* other)-> bool {
					return 0 == std::strncmp(s._begin, other, s.size());
				}
			private: // data
				const const_pointer _begin;
				const const_pointer _end = _begin;
			}; // class string_view
		} // namespace detail

		using string_view = detail::string_view;
	#endif

} // namespace vuh
