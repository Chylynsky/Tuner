#pragma once

namespace DSP
{
	template<typename value_t>
	constexpr value_t pi{ static_cast<value_t>(3.141592653589793) };

	// Sinc function
	template<typename T1>
	inline T1 sinc(T1 val) noexcept
	{
		return (val == static_cast<T1>(0)) ? static_cast<T1>(1) : std::sin(val) / val;
	}

	// Convert frequency to normalized omega (radians/sample)
	template<typename T1>
	inline T1 omega_norm(T1 f, T1 fs) noexcept
	{
		return static_cast<T1>(2) * pi<T1> * f / fs;
	}
}