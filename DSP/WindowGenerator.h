#pragma once
#include "DSPMath.h"

namespace DSP
{
	class WindowGenerator
	{
		// Generate Gaussian window coefficients
		template<typename _InIt>
		static void GenerateGaussianWindow(_InIt first, const _InIt last) noexcept;

		// Generate Triangular window coefficients
		template<typename _InIt>
		static void GenerateTriangularWindow(_InIt first, const _InIt last) noexcept;

		// Generate Welch window coefficients
		template<typename _InIt>
		static void GenerateWelchWindow(_InIt first, _InIt const last) noexcept;

		// Generate Hann window coefficients
		template<typename _InIt>
		static void GenerateHannWindow(_InIt first, _InIt const last) noexcept;

		// Generate Hamming window coefficients
		template<typename _InIt>
		static void GenerateHammingWindow(_InIt first, _InIt const last) noexcept;

		// Generate Blackman window coefficients
		template<typename _InIt>
		static void GenerateBlackmanWindow(_InIt first, _InIt const last) noexcept;

		// Generate Blckman-Nuttall window coefficients
		template<typename _InIt>
		static void GenerateBlackmanNuttallWindow(_InIt first, _InIt const last) noexcept;

		// Generate Blackman-Harris window coefficients
		template<typename _InIt>
		static void GenerateBlackmanHarrisWindow(_InIt first, _InIt const last) noexcept;

	public:

		enum class WindowType { 
			Gauss,
			Triangular,
			Welch,
			Hann,
			Hamming,
			Blackman,
			BlackmanNuttall,
			BlackmanHarris
		};

		// Generate the choosen window
		template<typename _InIt>
		static void Generate(WindowType type, _InIt first, const _InIt last) noexcept;
	};

	template<typename _InIt>
	inline void WindowGenerator::GenerateGaussianWindow(_InIt first, const _InIt last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;

		constexpr value_t alpha{ static_cast<value_t>(2.5) };
		const diff_t N = std::distance(first, last);
		const value_t step = (static_cast<value_t>(N) - static_cast<value_t>(1)) / static_cast<value_t>(2);

		value_t n = 0;
		std::for_each(first, last, [&n, &N, &alpha, &step](value_t& val) {
			val = std::exp(static_cast<value_t>(-0.5) * std::pow((alpha * n / step), 2));
			n++;
		});
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateTriangularWindow(_InIt first, const _InIt last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;

		const diff_t N = std::distance(first, last);

		value_t n = 0;
		std::for_each(first, last, [&n, &N](value_t& val) {
			val = static_cast<value_t>(1) - std::abs((n - static_cast<value_t>(N) / static_cast<value_t>(2)) / static_cast<value_t>(N));
			n++;
		});
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateWelchWindow(_InIt first, _InIt const last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;

		const diff_t N = std::distance(first, last);

		value_t n = 0;
		std::for_each(first, last, [&n, &N](value_t& val) {
			val = static_cast<value_t>(1) - std::pow((n - static_cast<value_t>(N) / static_cast<value_t>(2)) / (static_cast<value_t>(N) / static_cast<value_t>(2)), 2);
			n++;
		});
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateHannWindow(_InIt first, _InIt const last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;

		const diff_t N = std::distance(first, last);

		value_t n = 0;
		std::for_each(first, last, [&n, &N](value_t& val) {
			val = std::pow(std::sin((pi<value_t> * n / static_cast<value_t>(N))), 2);
			n++;
		});
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateHammingWindow(_InIt first, _InIt const last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;

		constexpr value_t a0{ static_cast<value_t>(25) / static_cast<value_t>(46) };
		constexpr value_t a1{ 1.0 - a0 };

		const diff_t N = std::distance(first, last);

		value_t n = 0;
		std::for_each(first, last, [&n, &N, &a0, &a1](value_t& val) {
			val = a0 + a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N));
			n++;
		});
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateBlackmanWindow(_InIt first, _InIt const last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;

		constexpr value_t a0{ static_cast<value_t>(7938) / static_cast<value_t>(18608) };
		constexpr value_t a1{ static_cast<value_t>(9240) / static_cast<value_t>(18608) };
		constexpr value_t a2{ static_cast<value_t>(1430) / static_cast<value_t>(18608) };

		const diff_t N = std::distance(first, last);

		value_t n = 0;
		std::for_each(first, last, [&n, &N, &a0, &a1, &a2](value_t& val) {
			val = a0 -
				a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N)) +
				a2 * std::cos(static_cast<value_t>(4) * pi<value_t> * n / static_cast<value_t>(N));
			n++;
		});
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateBlackmanNuttallWindow(_InIt first, const _InIt last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;

		constexpr value_t a0{ static_cast<value_t>(0.3635819) };
		constexpr value_t a1{ static_cast<value_t>(0.4891775) };
		constexpr value_t a2{ static_cast<value_t>(0.1365995) };
		constexpr value_t a3{ static_cast<value_t>(0.0106411) };

		const diff_t N = std::distance(first, last);
		
		value_t n = 0;
		std::for_each(first, last, [&n, &N, &a0, &a1, &a2, &a3](value_t& val) {
			val = a0 -
				a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N)) +
				a2 * std::cos(static_cast<value_t>(4) * pi<value_t> * n / static_cast<value_t>(N)) -
				a3 * std::cos(static_cast<value_t>(6) * pi<value_t> * n / static_cast<value_t>(N));
			n++;
		});
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateBlackmanHarrisWindow(_InIt first, const _InIt last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;

		constexpr value_t a0{ static_cast<value_t>(0.35875) };
		constexpr value_t a1{ static_cast<value_t>(0.48829) };
		constexpr value_t a2{ static_cast<value_t>(0.14128) };
		constexpr value_t a3{ static_cast<value_t>(0.01168) };

		const diff_t N = std::distance(first, last);

		value_t n = 0;
		std::for_each(first, last, [&n, &N, &a0, &a1, &a2, &a3](value_t& val) {
			val = a0 -
				a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * static_cast<value_t>(n) / static_cast<value_t>(N)) +
				a2 * std::cos(static_cast<value_t>(4) * pi<value_t> * static_cast<value_t>(n) / static_cast<value_t>(N)) -
				a3 * std::cos(static_cast<value_t>(6) * pi<value_t> * static_cast<value_t>(n) / static_cast<value_t>(N));
			n++;
		});
	}

	template<typename _InIt>
	inline void WindowGenerator::Generate(WindowGenerator::WindowType type, _InIt first, const _InIt last) noexcept
	{
		static_assert(std::is_floating_point<std::iterator_traits<_InIt>::value_type>(), "value_t must be of a floating point type.");

		switch (type) {
		case WindowType::Gauss:				WindowGenerator::GenerateGaussianWindow(first, last);			break;
		case WindowType::Triangular:		WindowGenerator::GenerateTriangularWindow(first, last);			break;
		case WindowType::Welch:				WindowGenerator::GenerateWelchWindow(first, last);				break;
		case WindowType::Hann:				WindowGenerator::GenerateHannWindow(first, last);				break;
		case WindowType::Hamming:			WindowGenerator::GenerateHammingWindow(first, last);			break;
		case WindowType::Blackman:			WindowGenerator::GenerateBlackmanWindow(first, last);			break;
		case WindowType::BlackmanNuttall:	WindowGenerator::GenerateBlackmanNuttallWindow(first, last);	break;
		case WindowType::BlackmanHarris:	WindowGenerator::GenerateBlackmanHarrisWindow(first, last);		break;
		default: break;
		}
	}
}