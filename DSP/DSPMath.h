#pragma once
#include <algorithm>
#include <cmath>
#include <execution>
#include "DSPTypeTraits.h"

namespace DSP
{
	template<typename T>
	inline constexpr T pi{ static_cast<T>(3.141592653589793238462643383279502884197169L) };

	// Sinc function
	template<typename T>
	inline T sinc(T val) noexcept
	{
		return (val == static_cast<T>(0)) ? static_cast<T>(1) : std::sin(val) / val;
	}

	// Convert frequency to normalized omega (radians/sample)
	template<typename T>
	inline T omega_norm(T f, T fs) noexcept
	{
		return static_cast<T>(2) * pi<T> * f / fs;
	}

	template<typename _FwdIt1, typename _FwdIt2, typename _FwdIt3>
	inline void MultiplyPointwise(_FwdIt1 first1, _FwdIt1 last1, _FwdIt2 first2, _FwdIt3 dest)
	{
		using value_t = Iterator_value_type<_FwdIt1>;

		static_assert(Is_same<value_t, Iterator_value_type<_FwdIt2>>, "Different value types.");
		static_assert(Is_same<value_t, Iterator_value_type<_FwdIt3>>, "Different value types.");

		std::transform(std::execution::par, first1, last1, first2, dest, std::multiplies<value_t>());
	}
}