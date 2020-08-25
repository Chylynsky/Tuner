#pragma once
#include "fftw3.h"

namespace DSP
{
	template<bool Test, typename _Ty = void>
	using Enable_if = typename std::enable_if_t<Test, _Ty>;

	template<typename _Ty1, typename _Ty2>
	inline constexpr bool Is_same = std::is_same_v<_Ty1, _Ty2>;

	template<typename _It>
	using Iterator_value_type = typename std::iterator_traits<_It>::value_type;

	template<typename _It>
	using Iterator_difference_type = typename std::iterator_traits<_It>::difference_type;

	template<typename _Ty>
	inline constexpr bool Is_floating_point = std::is_floating_point_v<_Ty>;

	template<typename _Ty>
	inline constexpr bool Is_float = Is_same<_Ty, float>;

	template<typename _Ty>
	inline constexpr bool Is_double = Is_same<_Ty, double>;

	template<typename _Ty>
	inline constexpr bool Is_long_double = Is_same<_Ty, long double>;

	template<typename _Ty, typename _It>
	inline constexpr bool Is_value_type_complex = Is_same<std::complex<_Ty>, std::iterator_traits<_It>::value_type>;

	template<typename _It>
	inline constexpr bool Is_value_type_floating_point = Is_floating_point<Iterator_value_type<_It>>;

	template<typename _Ty>
	struct Fftw_plan
	{
	};

	template<>
	struct Fftw_plan<float>
	{
		using type = typename fftwf_plan;
	};

	template<>
	struct Fftw_plan<double>
	{
		using type = typename fftw_plan;
	};

	template<>
	struct Fftw_plan<long double>
	{
		using type = typename fftwl_plan;
	};

	template<typename _Ty>
	using fftw_plan_type = typename Fftw_plan<_Ty>::type;
}