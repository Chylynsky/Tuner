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
		static constexpr size_t SAMPLES_TO_ANALYZE{ 1 << 15 };

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

		// Map where key: frequency, value: note
		const std::map<float, std::string> noteFrequencies;
		// Result of Fast Fourier Transform
		std::array<std::complex<float>, SAMPLES_TO_ANALYZE / 2ULL + 1ULL> fftResult;
		// Gaussian window coefficients
		std::array<float, SAMPLES_TO_ANALYZE> windowCoefficients;

		// Analysis thread control
		std::atomic<bool> quitAnalysis;
		// Analysis runs on its own thread
		std::thread pitchAnalysisThread;

		// Audio recording device
		AudioInput audioInput;
		// Required by FFTW
		fftwf_plan fftPlan;

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
		//  - std::function<void(const std::string&, float, float)> callback - callback function object, where
		//	- const std::string& - reference to the nearest note
		//	- float - difference in cents between the frequencies of the nearest note and current signal
		//	- float - frequency of the signal
		winrt::Windows::Foundation::IAsyncAction Run(std::function<void(const std::string&, float, float)> callback) noexcept;
	};

	template<typename iter>
	float PitchAnalyzer::GetFirstHarmonic(iter first, iter last, unsigned int sampling_freq) const noexcept
	{
		std::multimap<float, float> LUT;
		size_t n = minFrequency * SAMPLES_TO_ANALYZE / sampling_freq;

		// Check only frequencies in specified range
		first += n;
		const auto maxFreqIter = first + 1 + maxFrequency * SAMPLES_TO_ANALYZE / sampling_freq;

		while (first != maxFreqIter) {
			LUT.insert(std::make_pair(std::abs(*first), n * sampling_freq / SAMPLES_TO_ANALYZE));
			first++;
			n++;
		}

		return LUT.rbegin()->second;
	}

	// Sound analysis is performed on its own thread
	inline winrt::Windows::Foundation::IAsyncAction PitchAnalyzer::Run(std::function<void(const std::string&, float, float)> callback) noexcept
	{
		co_await audioInput.Initialize();

		audioInput.SetAudioBufferSize(SAMPLES_TO_ANALYZE);

		fftPlan = fftwf_plan_dft_r2c_1d(
			static_cast<int>(SAMPLES_TO_ANALYZE),
			audioInput.GetRawPtr(),
			reinterpret_cast<fftwf_complex*>(fftResult.data()),
			FFTW_MEASURE);

		DSP::WindowGenerator::Generate(DSP::WindowGenerator::WindowType::Blackman, windowCoefficients.begin(), windowCoefficients.end());
		pitchAnalysisThread = std::thread(&PitchAnalyzer::Analyze, this, callback);

		// Start recording input
		audioInput.Start();
	}
}