#pragma once

namespace DSP
{
	template<typename value_t>
	constexpr value_t pi{ static_cast<value_t>(3.141592653589793) };

	template<typename T>
	inline decltype(auto) sinc(T val) noexcept
	{
		return (val == static_cast<T>(0)) ? 1 : std::sin(val) / val;
	}
}