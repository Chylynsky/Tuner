#pragma once

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
}