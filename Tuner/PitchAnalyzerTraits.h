#pragma once
#include <type_traits>

namespace winrt::Tuner::implementation
{
	template<typename _Ty>
	inline constexpr bool Is_positive_power_of_2(_Ty value)
	{
		if constexpr (!std::is_arithmetic_v<_Ty>)
			return false;

		if constexpr (std::is_floating_point_v<_Ty>)
			return static_cast<unsigned long>(value) && !(static_cast<unsigned long>(value) & (static_cast<unsigned long>(value) - 1));
		else
			return value && !(value & (value - 1));
	}
}