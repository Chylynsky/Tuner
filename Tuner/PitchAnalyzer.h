#pragma once
#include "AudioInput.h"
#include "PitchAnalysisBuffer.h"
#include "WindowGenerator.h"
#include "FilterGenerator.h"

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

		std::array<PitchAnalysisBuffer, 2> pitchAnalysisBufferArray;
		std::queue<PitchAnalysisBuffer*> pitchAnalysisBufferQueue;

		SoundAnalyzedCallback soundAnalyzedCallback;

		// Template function that takes an input container of std::complex<T>, being the result of FFT and 
		// returns the frequency of the first harmonic.
		template<typename iter>
		float GetFirstHarmonic(iter first, iter last, uint32_t samplingFrequency) const noexcept;

		// Returns an index of a sample with the highest peak
		template<typename iter, typename diff_t = typename std::iterator_traits<iter>::difference_type>
		diff_t GetPeakQuefrency(iter first, iter last, uint32_t samplingFrequency) const noexcept;

		template<typename T, typename T2>
		float QuefrencyToFrequncy(T quefrency, T2 samplingFrequency) const noexcept;

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

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	//										TEMPLATE DEFINITIONS
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	template<typename iter>
	float PitchAnalyzer::GetFirstHarmonic(iter first, iter last, uint32_t samplingFrequency) const noexcept
	{
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		// Number of samples
		const diff_t N = std::distance<iter>(first, last);
		// Iterator to the upper frequency boundary
		const iter maxFreqIter = std::next(first, static_cast<diff_t>(1U + static_cast<uint32_t>(maxFrequency) * N / samplingFrequency));
		
		// Index of the sample representing lower frequency bound
		uint32_t n = static_cast<uint32_t>(minFrequency) * N / samplingFrequency;

		std::pair<float, float> highestAmplFreq{ 0.0, 0.0 };
		std::pair<float, float> tmpAmplFreq{ 0.0, 0.0 };

		// Check only frequencies in specified range
		first += n;

		while (first != maxFreqIter) {
			tmpAmplFreq = std::make_pair(std::abs(*first), n * samplingFrequency / N);
			if (tmpAmplFreq.first > highestAmplFreq.first) {
				highestAmplFreq = tmpAmplFreq;
			}
			first++;
			n++;
		}

		return highestAmplFreq.second;
	}

	template<typename iter, typename diff_t>
	diff_t PitchAnalyzer::GetPeakQuefrency(iter first, iter last, uint32_t samplingFrequency) const noexcept
	{
		// Number of samples
		const diff_t N = std::distance<iter>(first, last);
		// Iterator to the upper quefrency bound
		const iter maxQuefIter = first + static_cast<diff_t>(samplingFrequency / static_cast<diff_t>(minFrequency));

		assert(maxQuefIter < last);

		// Index of the lower quefrency bound
		diff_t n = static_cast<diff_t>(samplingFrequency) / static_cast<diff_t>(maxFrequency);
		// Advance to the lower quefrency bound
		first += n;

		std::pair<float, diff_t> highestAmplQuef{ 0.0, 0 };
		std::pair<float, diff_t> currAmplQuef{ 0.0, 0 };

		while (first != maxQuefIter) {
			currAmplQuef = std::make_pair(std::abs(*first), samplingFrequency / n);
			if (currAmplQuef.first > highestAmplQuef.first) {
				highestAmplQuef = currAmplQuef;
			}
			first++;
			n++;
		}

		return highestAmplQuef.second;;
	}

	template<typename T, typename T2>
	inline float PitchAnalyzer::QuefrencyToFrequncy(T quefrency, T2 samplingFrequency) const noexcept
	{
		return static_cast<float>(samplingFrequency) / static_cast<float>(quefrency);
	}
}