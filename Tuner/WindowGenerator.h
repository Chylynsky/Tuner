#pragma once
class WindowGenerator
{
	// Generate Gaussian window coefficients
	template<typename iter, typename T = float>
	static void GenerateGaussianWindow(iter first, iter last, const T alpha = 2.5) noexcept;

	// Generate Gaussian window coefficients
	template<typename iter>
	static void GeneratBlackmanWindow(iter first, iter last) noexcept;

public:

	enum class WindowType { Blackman, Gauss };

	template<typename iter>
	static void Generate(iter first, iter last, WindowType type) noexcept;
};

template<typename iter, typename T>
inline void WindowGenerator::GenerateGaussianWindow(iter first, iter last, const T alpha) noexcept
{
	// Half of the max window index
	const T step = (last - first - 1) / static_cast<T>(2);
	T n = -step;

	while (first != last)
	{
		*(first) = std::exp(static_cast<T>(-0.5) * std::pow((alpha * n / step), 2));
		first++;
		n++;
	}
}

template<typename iter>
inline void WindowGenerator::GeneratBlackmanWindow(iter first, iter last) noexcept
{
}

template<typename iter>
inline void WindowGenerator::Generate(iter first, iter last, WindowType type) noexcept
{
	switch (type)
	{
	case WindowType::Blackman: WindowGenerator::GenerateBlackmanWindow(first, last); break;
	case WindowType::Gauss: WindowGenerator::GenerateGaussianWindow(first, last); break;
	default: break;
	}
}
