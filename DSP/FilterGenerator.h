#pragma once
#include "DSPMath.h"
#include "WindowGenerator.h"

namespace DSP
{
	template<typename T, typename T2, typename iter>
	void GenerateBandPassFIR(T fc1, T fc2, T2 samplingFreq, iter first, const iter last, WindowGenerator::WindowType windowType = WindowGenerator::WindowType::Blackman)
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");

		// Pre-calculate formula for normalized angular frequency
		const value_t a0 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc1) / static_cast<value_t>(samplingFreq);
		const value_t a1 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc2) / static_cast<value_t>(samplingFreq);

		diff_t N = std::distance(first, last);
		diff_t NHalf = N / static_cast<diff_t>(2);

		WindowGenerator::Generate(windowType, first, last);

		for (diff_t n = 0; first != last; n++, first++) {
			value_t nDelayed = static_cast<value_t>(n - NHalf);
			*first *= a1 / pi<value_t> * sinc(nDelayed * a1) - a0 / pi<value_t> * sinc(nDelayed * a0);
		}
	}

	template<typename T, typename T2, typename iter>
	void GenerateLowPassFIR(T fc, T2 samplingFreq, iter first, const iter last, WindowGenerator::WindowType windowType = WindowGenerator::WindowType::Blackman)
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");

		// Pre-calculate formula for normalized angular frequency
		const value_t a0 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc) / static_cast<value_t>(samplingFreq);

		diff_t N = std::distance(first, last);
		diff_t NHalf = N / static_cast<diff_t>(2);

		WindowGenerator::Generate(windowType, first, last);

		for (diff_t n = 0; first != last; n++, first++) {
			*first *= a0 / pi<value_t> * sinc(static_cast<value_t>(n - NHalf) * a0);
		}
	}

	template<typename T, typename T2, typename iter>
	void GenerateHighPassFIR(T fc, T2 samplingFreq, iter first, const iter last, WindowGenerator::WindowType windowType = WindowGenerator::WindowType::Blackman)
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");

		// Pre-calculate formula for normalized angular frequency
		const value_t a0 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc) / static_cast<value_t>(samplingFreq);

		diff_t N = std::distance(first, last);
		diff_t NHalf = N / static_cast<diff_t>(2);

		WindowGenerator::Generate(windowType, first, last);

		for (diff_t n = 0; first != last; n++, first++) {
			value_t nDelayed = n - NHalf;
			*first *= sinc(nDelayed) - a0 / pi<value_t> * sinc(nDelayed * a0);
		}
	}
}