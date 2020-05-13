#pragma once
#include "DSPMath.h"
#include "WindowGenerator.h"

namespace DSP
{
	template<typename T, typename iter>
	void GenerateBandpassFIR(T fc1, T fc2, T samplingFreq, iter first, iter last, WindowGenerator::WindowType windowType = WindowGenerator::WindowType::Blackman)
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");

		// Pre-calculate formula for normalized angular frequency divided by pi
		const value_t a0 = 2 * static_cast<value_t>(fc1) / static_cast<value_t>(samplingFreq);
		const value_t a1 = 2 * static_cast<value_t>(fc2) / static_cast<value_t>(samplingFreq);

		diff_t N = std::distance(first, last);
		diff_t NHalf = N / static_cast<diff_t>(2);
		diff_t n = static_cast<diff_t>(0);

		WindowGenerator::Generate(windowType, first, last);

		while (first != last) {
			value_t nDelayed = n - NHalf;
			*first *= a1 * sinc(nDelayed * a1) - a0 * sinc(nDelayed * a0);
			std::advance(first, 1);
			n++;
		}
	}
}