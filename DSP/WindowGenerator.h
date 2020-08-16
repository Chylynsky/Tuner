#pragma once
#include "DSPMath.h"

namespace DSP
{
	class WindowGenerator
	{
		template<typename _It>
		static inline constexpr bool is_input_iterator = 
			std::is_same_v<std::iterator_traits<_It>::iterator_category, std::input_iterator_tag> ||
			std::is_same_v<std::iterator_traits<_It>::iterator_category, std::forward_iterator_tag> ||
			std::is_same_v<std::iterator_traits<_It>::iterator_category, std::bidirectional_iterator_tag> ||
			std::is_same_v<std::iterator_traits<_It>::iterator_category, std::random_access_iterator_tag>;

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
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");
		static_assert(is_input_iterator<_InIt>, "_InIt must be input iterator.");

		constexpr value_t alpha{ static_cast<value_t>(2.5) };
		const diff_t N = std::distance<_InIt>(first, last);
		const value_t step = (static_cast<value_t>(N) - static_cast<value_t>(1)) / static_cast<value_t>(2);

		for (value_t n = 0; first != last; first++, n++) {
			*(first) = std::exp(static_cast<value_t>(-0.5) * std::pow((alpha * n / step), 2));
		}
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateTriangularWindow(_InIt first, const _InIt last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");
		static_assert(is_input_iterator<_InIt>, "_InIt must be input iterator.");

		const diff_t N = std::distance<_InIt>(first, last);

		for (value_t n = 0; first != last; first++, n++) {
			*first = static_cast<value_t>(1) - std::abs((n - static_cast<value_t>(N) / static_cast<value_t>(2)) / static_cast<value_t>(N));
		}
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateWelchWindow(_InIt first, _InIt const last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");
		static_assert(is_input_iterator<_InIt>, "_InIt must be input iterator.");

		const diff_t N = std::distance<_InIt>(first, last);

		for (value_t n = 0; first != last; first++, n++) {
			*first = static_cast<value_t>(1) - std::pow((n - static_cast<value_t>(N) / static_cast<value_t>(2)) / (static_cast<value_t>(N) / static_cast<value_t>(2)), 2);
		}
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateHannWindow(_InIt first, _InIt const last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");
		static_assert(is_input_iterator<_InIt>, "_InIt must be input iterator.");

		const diff_t N = std::distance<_InIt>(first, last);

		for (value_t n = 0; first != last; first++, n++) {
			*first = std::pow(std::sin((pi<value_t> * n / static_cast<value_t>(N))), 2);
		}
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateHammingWindow(_InIt first, _InIt const last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");
		static_assert(is_input_iterator < _InIt>, "_InIt must be input iterator.");

		constexpr value_t a0{ static_cast<value_t>(25) / static_cast<value_t>(46) };
		constexpr value_t a1{ 1 - a0 };

		const diff_t N = std::distance<_InIt>(first, last);

		for (value_t n = 0; first != last; first++, n++) {
			*first = a0 + a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N));
		}
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateBlackmanWindow(_InIt first, _InIt const last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");
		static_assert(is_input_iterator<_InIt>, "_InIt must be input iterator.");

		constexpr value_t a0{ static_cast<value_t>(7938) / static_cast<value_t>(18608) };
		constexpr value_t a1{ static_cast<value_t>(9240) / static_cast<value_t>(18608) };
		constexpr value_t a2{ static_cast<value_t>(1430) / static_cast<value_t>(18608) };

		const diff_t N = std::distance<_InIt>(first, last);

		for (value_t n = 0; first != last; first++, n++) {
			*first = a0 - 
				a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N)) + 
				a2 * std::cos(static_cast<value_t>(4) * pi<value_t> * n / static_cast<value_t>(N));
		}
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateBlackmanNuttallWindow(_InIt first, const _InIt last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");
		static_assert(is_input_iterator<_InIt>, "_InIt must be input iterator.");

		constexpr value_t a0{ static_cast<value_t>(0.3635819) };
		constexpr value_t a1{ static_cast<value_t>(0.4891775) };
		constexpr value_t a2{ static_cast<value_t>(0.1365995) };
		constexpr value_t a3{ static_cast<value_t>(0.0106411) };

		const diff_t N = std::distance<_InIt>(first, last);

		for (value_t n = 0; first != last; first++, n++) {
			*first = a0 - 
				a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N)) + 
				a2 * std::cos(static_cast<value_t>(4) * pi<value_t> * n / static_cast<value_t>(N)) - 
				a3 * std::cos(static_cast<value_t>(6) * pi<value_t> * n / static_cast<value_t>(N));
		}
	}

	template<typename _InIt>
	inline void WindowGenerator::GenerateBlackmanHarrisWindow(_InIt first, const _InIt last) noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;
		static_assert(std::is_floating_point<value_t>(), "value_t must be of a floating point type.");
		static_assert(is_input_iterator<_InIt>, "_InIt must be input iterator.");

		constexpr value_t a0{ static_cast<value_t>(0.35875) };
		constexpr value_t a1{ static_cast<value_t>(0.48829) };
		constexpr value_t a2{ static_cast<value_t>(0.14128) };
		constexpr value_t a3{ static_cast<value_t>(0.01168) };

		const diff_t N = std::distance<_InIt>(first, last);

		for (value_t n = 0; first != last; first++, n++) {
			*first = a0 - 
				a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N)) + 
				a2 * std::cos(static_cast<value_t>(4) * pi<value_t> * n / static_cast<value_t>(N)) - 
				a3 * std::cos(static_cast<value_t>(6) * pi<value_t> * n / static_cast<value_t>(N));
		}
	}

	template<typename _InIt>
	inline void WindowGenerator::Generate(WindowGenerator::WindowType type, _InIt first, const _InIt last) noexcept
	{
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