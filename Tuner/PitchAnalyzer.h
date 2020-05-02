#pragma once
#include "AudioInput.h"

// Avoid name collisions
#undef max
#undef min

class PitchAnalyzer
{
	// Number of samples that undergo the analysis
	const size_t samplesToAnalyze;
	// Requested frequency range
	const float minFrequency;
	const float maxFrequency;
	// Base note A4
	float baseNoteFrequency;
	// Map where key: frequency, value: note
	const std::map<float, std::string> noteFrequencies;
	// Audio recording device
	AudioInput dev;
	// Analysis runs on its own thread
	std::thread analysis;
	// Result of Fast Fourier Transform
	std::vector<std::complex<float>> fftResult;
	// Frequency of the highest amplitude
	float firstHarmonic;
	// Difference (in cents) between frequencies of the input signal and the nearest note
	float cents;

	std::string note;
	// Required by FFTW
	fftwf_plan fftPlan;
	std::atomic<bool> quit;

	// Analyzes input container of std::complex<T>, being the result of FFT. Returns frequency with the highest amplitude.
	// Requires iterators and the sampling frequency of the analysed signal.
	template<typename iterator>
	void GetFirstHarmonic(iterator first, iterator last, unsigned int sampling_freq)
	{
		static const int N = static_cast<int>(last - first);
		static std::vector<float> frequencies(N);
		static std::vector<float> amplitudes(N);
		
		for (int i = 0; i < N; i++)
		{
			// Get frequencies
			frequencies[i] = i * sampling_freq / N;
			// Get amplitude
			amplitudes[i] = std::abs(*(first + i));
		}

		auto maxAmplitudeIndex = std::max_element(amplitudes.begin(), amplitudes.end());
		firstHarmonic = frequencies[maxAmplitudeIndex - amplitudes.begin()];
	}

	// Get std::string with a name of the note, based on the given frequency
	void GetNote(float frequency);

	void Analyze(void* instance = nullptr, void (*callback)(void*, std::string&, float, float) = nullptr);

	std::map<float, std::string> InitializeNoteFrequenciesMap();

public:

	PitchAnalyzer(float A4 = 440.0f, float minFrequency = 20.0f, float maxFrequency = 8000.0f, size_t samplesToAnalyze = 1 << 17);
	~PitchAnalyzer();

	void Run(void* instance = nullptr, void (*callback)(void*, std::string&, float, float) = nullptr);
};