#pragma once

namespace DSP
{
	class WindowGenerator
	{
		template<typename T>
		inline static constexpr T pi{ static_cast<T>(3.141592653589793) };

		// Generate Gaussian window coefficients
		template<typename iter, typename T = float>
		static void GenerateGaussianWindow(iter first, iter last) noexcept;

		// Generate Blackman window coefficients
		template<typename iter, typename T = float>
		static void GeneratBlackmanWindow(iter first, iter last) noexcept;

		// Helper method for multiplying window coefficients with samples
		template<typename iter1, typename iter2>
		static void _ApplyWindow(iter1 samplesFirst, iter1 samplesLast, iter2 windowFirst) noexcept;

	public:

		enum class WindowType { 
			Blackman, 
			Gauss 
		};

		// Generate the choosen window
		template<typename iter>
		static void Generate(WindowType type, iter first, iter last) noexcept;

		// Apply window to a given signal in a given number of threads.
		// Window length must match with the number of samples.
		template<typename iter1, typename iter2>
		static void ApplyWindow(iter1 samplesFirst, iter1 samplesLast, iter2 windowFirst, size_t threadCount = 1) noexcept;
	};

	template<typename iter, typename T>
	inline void WindowGenerator::GenerateGaussianWindow(iter first, iter last) noexcept
	{
		constexpr T alpha = static_cast<T>(2.5);
		const T step = (last - first - static_cast<T>(1)) / static_cast<T>(2);
		T n = -step;

		while (first != last) {
			*(first) = std::exp(static_cast<T>(-0.5) * std::pow((alpha * n / step), 2));
			first++;
			n++;
		}
	}

	template<typename iter, typename T>
	inline void WindowGenerator::GeneratBlackmanWindow(iter first, iter last) noexcept
	{
		constexpr T a0 = static_cast<T>(7938) / static_cast<T>(18608);
		constexpr T a1 = static_cast<T>(9240) / static_cast<T>(18608);
		constexpr T a2 = static_cast<T>(1430) / static_cast<T>(18608);

		const T N = last - first;
		const T step = (N - 1) / static_cast<T>(2);
		T n = -step;

		while (first != last) {
			*first = a0 - a1 * std::cos(static_cast<T>(2) * pi<T> * n / N) + a2 * std::cos(static_cast<T>(4) * pi<T> * n / N);
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
		case WindowType::Blackman:	WindowGenerator::GeneratBlackmanWindow(first, last);	break;
		case WindowType::Gauss:		WindowGenerator::GenerateGaussianWindow(first, last);	break;
		default: break;
		}
	}

	template<typename iter1, typename iter2>
	inline void WindowGenerator::ApplyWindow(iter1 samplesFirst, iter1 samplesLast, iter2 windowFirst, size_t threadCount) noexcept
	{
		// Number of samples
		const size_t N = samplesLast - samplesFirst;

		// Number of samples must be higher and dividable by the number of threads.
		assert(N % threadCount == 0 && N > threadCount);

		const size_t step = N / threadCount;
		std::vector<std::thread> threadPool(threadCount);

		size_t i = 0;
		for (std::thread& thread : threadPool) {
			thread = std::thread([=]() {
				WindowGenerator::_ApplyWindow(samplesFirst + i * step, samplesFirst + (i + 1) * step, windowFirst + i * step);
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