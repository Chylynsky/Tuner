#pragma once

namespace DSP
{
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
}