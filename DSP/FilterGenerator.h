#pragma once
#include "DSPMath.h"
#include "WindowGenerator.h"

namespace DSP
{
	template<typename T1, typename T2, typename iter>
	void GenerateBandPassFIR(T1 fc1, T1 fc2, T2 samplingFreq, iter first, iter last, WindowGenerator::WindowType windowType = WindowGenerator::WindowType::Blackman)
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");

		// Pre-calculate formula for normalized angular frequency
		const value_t a0 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc1) / static_cast<value_t>(samplingFreq);
		const value_t a1 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc2) / static_cast<value_t>(samplingFreq);

		diff_t N = std::distance(first, last);
		diff_t NHalf = N / static_cast<diff_t>(2);
		diff_t n = static_cast<diff_t>(0);

		WindowGenerator::Generate(windowType, first, last);

		while (first != last) {
			value_t nDelayed = n - NHalf;
			*first *= a1 / pi<value_t> * sinc(nDelayed * a1) - a0 / pi<value_t> * sinc(nDelayed * a0);
			std::advance(first, 1);
			n++;
		}
	}

	template<typename T1, typename T2, typename iter>
	void GenerateLowPassFIR(T1 fc, T2 samplingFreq, iter first, iter last, WindowGenerator::WindowType windowType = WindowGenerator::WindowType::Blackman)
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");

		// Pre-calculate formula for normalized angular frequency
		const value_t a0 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc) / static_cast<value_t>(samplingFreq);

		diff_t N = std::distance(first, last);
		diff_t NHalf = N / static_cast<diff_t>(2);
		diff_t n = static_cast<diff_t>(0);

		WindowGenerator::Generate(windowType, first, last);

		while (first != last) {
			*first *= a0 / pi<value_t> * sinc((n - NHalf) * a0);
			std::advance(first, 1);
			n++;
		}
	}

	template<typename T1, typename T2, typename iter>
	void GenerateHighPassFIR(T1 fc, T2 samplingFreq, iter first, iter last, WindowGenerator::WindowType windowType = WindowGenerator::WindowType::Blackman)
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");

		// Pre-calculate formula for normalized angular frequency
		const value_t a0 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc) / static_cast<value_t>(samplingFreq);

		diff_t N = std::distance(first, last);
		diff_t NHalf = N / static_cast<diff_t>(2);
		diff_t n = static_cast<diff_t>(0);

		WindowGenerator::Generate(windowType, first, last);

		while (first != last) {
			value_t nDelayed = n - NHalf;
			*first *= sinc(nDelayed) - a0 / pi<value_t> * sinc(nDelayed * a0);
			std::advance(first, 1);
			n++;
		}
	}
}