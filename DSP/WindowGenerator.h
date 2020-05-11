#pragma once

namespace DSP
{
	class WindowGenerator
	{
		template<typename value_t>
		static constexpr value_t pi{ 3.141592653589793 };

		// Generate Gaussian window coefficients
		template<typename iter>
		static void GenerateGaussianWindow(iter first, iter last) noexcept;

		// Generate Triangular window coefficients
		template<typename iter>
		static void GenerateTriangularWindow(iter first, iter last) noexcept;

		// Generate Welch window coefficients
		template<typename iter>
		static void GenerateWelchWindow(iter first, iter last) noexcept;

		// Generate Hann window coefficients
		template<typename iter>
		static void GenerateHannWindow(iter first, iter last) noexcept;

		// Generate Hamming window coefficients
		template<typename iter>
		static void GenerateHammingWindow(iter first, iter last) noexcept;

		// Generate Blackman window coefficients
		template<typename iter>
		static void GenerateBlackmanWindow(iter first, iter last) noexcept;

		// Generate Blckman-Nuttall window coefficients
		template<typename iter>
		static void GenerateBlackmanNuttallWindow(iter first, iter last) noexcept;

		// Generate Blackman-Harris window coefficients
		template<typename iter>
		static void GenerateBlackmanHarrisWindow(iter first, iter last) noexcept;

		// Helper method for multiplying window coefficients with samples
		template<typename iter1, typename iter2>
		static void _ApplyWindow(iter1 samplesFirst, iter1 samplesLast, iter2 windowFirst) noexcept;

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
		template<typename iter>
		static void Generate(WindowType type, iter first, iter last) noexcept;

		// Apply window to a given signal in a given number of threads.
		// Window length must match with the number of samples.
		template<typename iter1, typename iter2>
		static void ApplyWindow(iter1 samplesFirst, iter1 samplesLast, iter2 windowFirst, uint32_t threadCount = 1) noexcept;
	};

	template<typename iter>
	inline void WindowGenerator::GenerateGaussianWindow(iter first, iter last) noexcept
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		constexpr value_t alpha{ 2.5 };
		const diff_t N = std::distance<iter>(first, last);
		const value_t step = (static_cast<value_t>(N) - static_cast<value_t>(1)) / static_cast<value_t>(2);

		value_t n = -step;
		while (first != last) {
			*(first) = std::exp(static_cast<value_t>(-0.5) * std::pow((alpha * n / step), 2));
			first++;
			n++;
		}
	}

	template<typename iter>
	inline void WindowGenerator::GenerateTriangularWindow(iter first, iter last) noexcept
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		const diff_t N = std::distance<iter>(first, last);

		value_t n = 0;
		while (first != last) {
			*first = static_cast<value_t>(1) - std::abs((n - static_cast<value_t>(N) / static_cast<value_t>(2)) / static_cast<value_t>(N));
			first++;
			n++;
		}
	}

	template<typename iter>
	inline void WindowGenerator::GenerateWelchWindow(iter first, iter last) noexcept
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		const diff_t N = std::distance<iter>(first, last);

		value_t n = 0;
		while (first != last) {
			*first = static_cast<value_t>(1) - std::pow((n - static_cast<value_t>(N) / static_cast<value_t>(2)) / (static_cast<value_t>(N) / static_cast<value_t>(2)), 2);
			first++;
			n++;
		}
	}

	template<typename iter>
	inline void WindowGenerator::GenerateHannWindow(iter first, iter last) noexcept
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		const diff_t N = std::distance<iter>(first, last);
		const value_t step = static_cast<value_t>(N - 1) / static_cast<value_t>(2);
		value_t n = -step;

		while (first != last) {
			*first = std::pow(std::sin((pi<value_t> * n / static_cast<value_t>(N))), 2);
			first++;
			n++;
		}
	}

	template<typename iter>
	inline void WindowGenerator::GenerateHammingWindow(iter first, iter last) noexcept
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		constexpr value_t a0 = static_cast<value_t>(25) / static_cast<value_t>(46);
		constexpr value_t a1 = 1 - a0;

		const diff_t N = std::distance<iter>(first, last);

		value_t n = 0;
		while (first != last) {
			*first = a0 + a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N));
			first++;
			n++;
		}
	}

	template<typename iter>
	inline void WindowGenerator::GenerateBlackmanWindow(iter first, iter last) noexcept
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		constexpr value_t a0 = static_cast<value_t>(7938) / static_cast<value_t>(18608);
		constexpr value_t a1 = static_cast<value_t>(9240) / static_cast<value_t>(18608);
		constexpr value_t a2 = static_cast<value_t>(1430) / static_cast<value_t>(18608);

		const diff_t N = std::distance<iter>(first, last);

		value_t n = 0;
		while (first != last) {
			*first = a0 - a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N)) + a2 * std::cos(static_cast<value_t>(4) * pi<value_t> * n / static_cast<value_t>(N));
			first++;
			n++;
		}
	}

	template<typename iter>
	inline void WindowGenerator::GenerateBlackmanNuttallWindow(iter first, iter last) noexcept
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		constexpr value_t a0 = 0.3635819;
		constexpr value_t a1 = 0.4891775;
		constexpr value_t a2 = 0.1365995;
		constexpr value_t a3 = 0.0106411;

		const diff_t N = std::distance<iter>(first, last);

		value_t n = 0;
		while (first != last) {
			*first = a0 - a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N)) + a2 * std::cos(static_cast<value_t>(4) * pi<value_t> * n / static_cast<value_t>(N)) - a3 * std::cos(static_cast<value_t>(6) * pi<value_t> * n / static_cast<value_t>(N));
			first++;
			n++;
		}
	}

	template<typename iter>
	inline void WindowGenerator::GenerateBlackmanHarrisWindow(iter first, iter last) noexcept
	{
		using value_t = typename std::iterator_traits<iter>::value_type;
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		constexpr value_t a0 = 0.35875;
		constexpr value_t a1 = 0.48829;
		constexpr value_t a2 = 0.14128;
		constexpr value_t a3 = 0.01168;

		const diff_t N = std::distance<iter>(first, last);

		value_t n = 0;
		while (first != last) {
			*first = a0 - a1 * std::cos(static_cast<value_t>(2) * pi<value_t> * n / static_cast<value_t>(N)) + a2 * std::cos(static_cast<value_t>(4) * pi<value_t> * n / static_cast<value_t>(N)) - a3 * std::cos(static_cast<value_t>(6) * pi<value_t> * n / static_cast<value_t>(N));
			first++;
			n++;
		}
	}

	template<typename iter1, typename iter2>
	inline void WindowGenerator::_ApplyWindow(iter1 samplesFirst, iter1 samplesLast, iter2 windowFirst) noexcept
	{
		while (samplesFirst != samplesLast) {
			*samplesFirst *= *windowFirst;
			samplesFirst++;
			windowFirst++;
		}
	}

	template<typename iter>
	inline void WindowGenerator::Generate(WindowType type, iter first, iter last) noexcept
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

	template<typename iter1, typename iter2>
	inline void WindowGenerator::ApplyWindow(iter1 samplesFirst, iter1 samplesLast, iter2 windowFirst, uint32_t threadCount) noexcept
	{
		using diff_t = typename std::iterator_traits<iter1>::difference_type;
		// Number of samples
		const diff_t N = std::distance<iter1>(samplesFirst, samplesLast);
		const diff_t step = N / static_cast<diff_t>(threadCount);

		// Number of samples must be higher and dividable by the number of threads.
		assert(N % threadCount == 0 && N > threadCount);

		std::vector<std::thread> threadPool(threadCount);

		diff_t i = 0;
		for (std::thread& thread : threadPool) {
			thread = std::thread([=]() {
				WindowGenerator::_ApplyWindow(
					std::next(samplesFirst, i * step), 
					std::next(samplesFirst, (i + 1) * step), 
					std::next(windowFirst, i * step));
				});
			i++;
		}

		for (std::thread& thread : threadPool) {
			if (thread.joinable()) {
				thread.join();
			}
		}
	}
}