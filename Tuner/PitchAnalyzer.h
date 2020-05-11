#pragma once
#include "AudioInput.h"
#include "PitchAnalysisBuffer.h"
#include "WindowGenerator.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

namespace winrt::Tuner::implementation
{
	class PitchAnalyzer
	{
		using SoundAnalyzedCallback = std::function<void(const std::string& note, float frequency, float innaccuracy)>;

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
		// Gaussian window coefficients
		std::vector<float> windowCoefficients;

		// Audio recording device
		AudioInput audioInput;

		std::array<PitchAnalysisBuffer, 4> pitchAnalysisBufferArray;
		std::queue<PitchAnalysisBuffer*> pitchAnalysisBufferQueue;

		SoundAnalyzedCallback soundAnalyzedCallback;

		//	Template function that takes an input container of std::complex<T>, being the result of FFT and 
		// returns the frequency of the first harmonic.
		template<typename iter>
		float GetFirstHarmonic(iter first, iter last, uint32_t sampling_freq) const noexcept;

		// Function takes frequency and returns PitchAnalysisResult object
		PitchAnalysisResult GetNote(float frequency) const noexcept;

		// Function performs harmonic analysis on input signal and calls the callback function (passed in PitchAnalyzer::Run()) 
		// for each analysis performed.
		void Analyze(PitchAnalysisBuffer* pitchAnalysisBuffer) noexcept;

		// Function used for filling the noteFrequencies std::map.
		std::map<float, std::string> InitializeNoteFrequenciesMap() noexcept;

		// AudioInput BufferFilled event callback
		void AudioInput_BufferFilled(AudioInput& sender, PitchAnalysisBuffer* args);

	public:

		explicit PitchAnalyzer(float A4 = 440.0f, float minFrequency = 20.0f, float maxFrequency = 8000.0f);
		~PitchAnalyzer();

		winrt::Windows::Foundation::IAsyncAction Run(SoundAnalyzedCallback soundAnalyzedCallback) noexcept;
	};

	template<typename iter>
	float PitchAnalyzer::GetFirstHarmonic(iter first, iter last, uint32_t sampling_freq) const noexcept
	{
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		// Number of samples
		const diff_t N = std::distance<iter>(first, last);
		// Iterator to the upper frequency boundary
		const iter maxFreqIter = std::next(first, static_cast<diff_t>(1U + static_cast<uint32_t>(maxFrequency) * N / sampling_freq));
		
		uint32_t n = static_cast<uint32_t>(minFrequency) * N / sampling_freq;

		std::pair<float, float> highestAmplFreq{ 0.0, 0.0 };
		std::pair<float, float> tmpAmplFreq{ 0.0, 0.0 };

		// Check only frequencies in specified range
		first += n;

		while (first != maxFreqIter) {
			tmpAmplFreq = { std::abs(*first), n * sampling_freq / N };
			if (tmpAmplFreq.first > highestAmplFreq.first) {
				highestAmplFreq = tmpAmplFreq;
			}
			first++;
			n++;
		}

		return highestAmplFreq.second;
	}

	// Sound analysis is performed on its own thread
	inline winrt::Windows::Foundation::IAsyncAction PitchAnalyzer::Run(PitchAnalyzer::SoundAnalyzedCallback soundAnalyzedCallback) noexcept
	{
		// Initialize AudioInput object
		co_await audioInput.Initialize();
		// Attach callback
		this->soundAnalyzedCallback = soundAnalyzedCallback;
		// Get first buffer from queue and attach it to AudioInput class object
		audioInput.AttachBuffer(pitchAnalysisBufferQueue.front());
		// Pass function as callback
		audioInput.BufferFilled(std::bind(&PitchAnalyzer::AudioInput_BufferFilled, this, std::placeholders::_1, std::placeholders::_2));
		// Generate window coefficients
		DSP::WindowGenerator::Generate(DSP::WindowGenerator::WindowType::BlackmanHarris, windowCoefficients.begin(), windowCoefficients.end());
		// Start recording input
		audioInput.Start();
	}
}