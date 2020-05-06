#pragma once
#include "AudioInput.h"
#include "WindowGenerator.h"

// Avoid name collisions
#undef max
#undef min

namespace winrt::Tuner::implementation
{
	class PitchAnalyzer
	{
		static constexpr size_t SAMPLES_TO_ANALYZE{ 1 << 14 };

		struct PitchAnalysisResult
		{
			const std::string& note;
			float cents;
		};

		// Requested frequency range
		const float minFrequency;
		const float maxFrequency;
		// Base note A4
		float baseNoteFrequency;
		// Sample rate
		size_t sampleRate;
		// Map where key: frequency, value: note
		const std::map<float, std::string> noteFrequencies;
		// Audio recording device
		AudioInput dev;
		// Analysis runs on its own thread
		std::thread analysis;
		// Result of Fast Fourier Transform
		std::vector<std::complex<float>> fftResult;
		// Required by FFTW
		fftwf_plan fftPlan;
		// Analysis thread control
		std::atomic<bool> quit;
		// Gaussian window coefficients
		std::array<float, SAMPLES_TO_ANALYZE> windowCoefficients;

		//	Template function that takes an input container of std::complex<T>, being the result of FFT and 
		// returns the frequency of the first harmonic.
		template<typename iter>
		float GetFirstHarmonic(iter first, iter last, uint32_t sampling_freq) const noexcept;

		// Function that takes an input container of std::complex<T>, being the result of FFTand returns
		// a filled PitchAnalysisResult struct.
		PitchAnalysisResult GetNote(float frequency) const noexcept;

		// Function performs harmonic analysis on input signal and calls the callback function for each 
		// analysis performed.
		// - std::function<void(const std::string&, float, float)> callback - callback function object, where
		//	- const std::string& - reference to the nearest note
		//	- float - difference in cents between the frequencies of the nearest note and current signal
		//	- float - frequency of the signal
		void Analyze(std::function<void(const std::string&, float, float)> callback) noexcept;

		// Function used for filling the noteFrequencies std::map.
		std::map<float, std::string> InitializeNoteFrequenciesMap() noexcept;

	public:

		explicit PitchAnalyzer(float A4 = 440.0f, float minFrequency = 20.0f, float maxFrequency = 8000.0f);
		~PitchAnalyzer();

		// Function starts an analysis in seperate thread.
		// - std::function<void(const std::string&, float, float)> callback - callback function object, where
		//	- const std::string& - reference to the nearest note
		//	- float - difference in cents between the frequencies of the nearest note and current signal
		//	- float - frequency of the signal
		void Run(std::function<void(const std::string&, float, float)> callback) noexcept;
	};

	template<typename iter>
	float PitchAnalyzer::GetFirstHarmonic(iter first, iter last, unsigned int sampling_freq) const noexcept
	{
		static const int N = static_cast<int>(last - first);
		static std::vector<float> frequencies(N);
		static std::vector<float> amplitudes(N);

		for (int i = 0; i < N; i++) {
			// Get frequencies
			frequencies[i] = i * sampling_freq / N;
			// Get amplitude
			amplitudes[i] = std::abs(*(first + i));
		}

		auto maxAmplitudeIndex = std::max_element(amplitudes.begin(), amplitudes.end());
		return frequencies[maxAmplitudeIndex - amplitudes.begin()];
	}

	// Sound analysis is performed on its own thread
	inline void PitchAnalyzer::Run(std::function<void(const std::string&, float, float)> callback) noexcept
	{
		analysis = std::thread(&PitchAnalyzer::Analyze, this, callback);
	}
}