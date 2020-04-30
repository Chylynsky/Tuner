#pragma once
#include "AudioInput.h"

// Avoid name collisions
#undef max
#undef min

class PitchAnalyzer
{
	// Number of samples that undergo the analysis
	static const long samplesToAnalyze;
	// Requested frequency range
	static const float minFrequency;
	static const float maxFrequency;
	// Minimal amplitude to be displayed
	//static const float minAmplitude;
	// Full octave
	//static const std::vector<std::string> octave;

	static const std::map<float, std::string> octave;
	// Constant needed for note frequencies calculation
	static const float a;
	// Base note A4 = 440Hz
	int A4;
	
	// Audio recording device
	AudioInput dev;
	// Analysis runs on its own thread
	std::thread analysis;
	// Result of Fast Fourier Transform
	std::vector<std::complex<float>> fftResult;
	// Frequency of the highest amplitude
	float firstHarmonic;
	// Maximum amplitude recorded in every measurement
	float maxAmplitude;
	// Value between -0.5 and 0.5, 0 means accurate note
	float inaccuracy;
	std::string note;
	// Required by FFTW
	fftwf_plan fftPlan;
	bool quit;


	// Analyzes input container of std::complex<T>, being the result of DFT. Returns frequency with the highest amplitude.
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
		maxAmplitude = *maxAmplitudeIndex;
		firstHarmonic = frequencies[maxAmplitudeIndex - amplitudes.begin()];
	}

	// Get std::string with a name of the note, based on the given frequency
	void GetNote(float frequency);

	void Analyze(void* instance = nullptr, void (*callback)(void*, std::string&, float, float) = nullptr);

public:

	PitchAnalyzer();
	~PitchAnalyzer();

	void Run(void* instance = nullptr, void (*callback)(void*, std::string&, float, float) = nullptr);
};