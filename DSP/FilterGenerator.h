#pragma once
#include "DSPMath.h"
#include "WindowGenerator.h"

namespace DSP
{
	template<typename T, typename T2, typename _InIt>
	void GenerateBandPassFIR(T fc1, T fc2, T2 samplingFreq, _InIt first, const _InIt last, WindowGenerator::WindowType windowType = WindowGenerator::WindowType::BlackmanHarris)
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");

		// Pre-calculate formula for normalized angular frequency
		const value_t a0 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc1) / static_cast<value_t>(samplingFreq);
		const value_t a1 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc2) / static_cast<value_t>(samplingFreq);

		diff_t N = std::distance(first, last);
		diff_t NHalf = N / static_cast<diff_t>(2);

		WindowGenerator::Generate(windowType, first, last);

		diff_t n = 0;
		std::for_each(first, last, [&n, &N, &NHalf, &a0, &a1](value_t& val) {
			value_t nDelayed = static_cast<value_t>(n - NHalf);
			val *= a1 / pi<value_t> * sinc(nDelayed * a1) - a0 / pi<value_t> * sinc(nDelayed * a0);
			n++;
		});
	}

	template<typename T, typename T2, typename _InIt>
	void GenerateLowPassFIR(T fc, T2 samplingFreq, _InIt first, const _InIt last, WindowGenerator::WindowType windowType = WindowGenerator::WindowType::BlackmanHarris)
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");

		// Pre-calculate formula for normalized angular frequency
		const value_t a0 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc) / static_cast<value_t>(samplingFreq);

		diff_t N = std::distance(first, last);
		diff_t NHalf = N / static_cast<diff_t>(2);

		WindowGenerator::Generate(windowType, first, last);

		diff_t n = 0;
		std::for_each(first, last, [&n, &N, &NHalf, &a0](value_t& val) {
			val *= a0 / pi<value_t> * sinc(static_cast<value_t>(n - NHalf) * a0);
			n++;
		});
	}

	template<typename T, typename T2, typename _InIt>
	void GenerateHighPassFIR(T fc, T2 samplingFreq, _InIt first, const _InIt last, WindowGenerator::WindowType windowType = WindowGenerator::WindowType::BlackmanHarris)
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");

		// Pre-calculate formula for normalized angular frequency
		const value_t a0 = static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(fc) / static_cast<value_t>(samplingFreq);

		diff_t N = std::distance(first, last);
		diff_t NHalf = N / static_cast<diff_t>(2);

		WindowGenerator::Generate(windowType, first, last);

		diff_t n = 0;
		std::for_each(first, last, [&n, &N, &NHalf, &a0](value_t& val) {
			value_t nDelayed = static_cast<value_t>(n - NHalf);
			val *= sinc(nDelayed) - a0 / pi<value_t> * sinc(nDelayed * a0);
			n++;
		});
	}
}