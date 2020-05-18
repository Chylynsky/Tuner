#pragma once
#include "AudioInput.h"
#include "FilterGenerator.h"

#define LOG_ANALYSIS

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
	public:
		using sample_t = float;
		using complex_t = std::complex<sample_t>;
		using AudioBuffer = std::vector<sample_t>;
		using AudioBufferIteratorPair = std::pair<sample_t*, sample_t*>;
		using FFTResultBuffer = std::vector<complex_t>;
		using NoteFrequenciesMap = std::map<sample_t, std::string>;
		using AudioBufferArray = std::array<AudioBuffer, 4>;
		using AudioBufferQueue = std::queue<AudioBufferIteratorPair>;
		using SoundAnalyzedCallback = std::function<void(const std::string& note, float frequency, float innaccuracy)>;

		struct PitchAnalysisResult
		{
			const std::string& note;
			float cents;
		};

		static constexpr uint32_t AUDIO_BUFFER_SIZE{ 1 << 16 };
		static constexpr uint32_t FILTER_SIZE{ 1 << 12 };
		static constexpr uint32_t OUTPUT_SIGNAL_SIZE{ AUDIO_BUFFER_SIZE + FILTER_SIZE - 1U };
		static constexpr uint32_t FFT_RESULT_SIZE{ OUTPUT_SIGNAL_SIZE / 2U + 1U };

		// Requested frequency range
		static constexpr float MIN_FREQUENCY{ 80.0f };
		static constexpr float MAX_FREQUENCY{ 1200.0f };
		// A4 base note frequency
		static constexpr float BASE_NOTE_FREQUENCY{ 440.0f };

		explicit PitchAnalyzer();
		~PitchAnalyzer();

		// Attach function that gets called when sound analysis is completed
		void SoundAnalyzed(SoundAnalyzedCallback soundAnalyzedCallback) noexcept;

		winrt::Windows::Foundation::IAsyncAction Run() noexcept;

	private:

		// Event handlers
		SoundAnalyzedCallback soundAnalyzedCallback;

		std::mutex pitchAnalyzerMtx;

		fftwf_plan fftPlan;
		FFTResultBuffer fftResult;

		// Map where key - frequency, value - note
		const NoteFrequenciesMap noteFrequencies{ InitializeNoteFrequenciesMap() };

		// FIR filter parameters
		AudioBuffer filterCoeff;
		FFTResultBuffer filterFreqResponse;
		
		// Convolution result
		AudioBuffer outputSignal;

		// Audio recording device
		AudioInput audioInput;
		
		// Array of buffers that hold recorded samples
		AudioBufferArray audioBuffersArray;
		// Queue of available buffers
		AudioBufferQueue audioBufferIterPairQueue;

		// Template function that takes an input container of std::complex<T>, being the result of FFT and 
		// returns the frequency of the first harmonic.
		template<typename iter>
		float GetFirstHarmonic(iter first, iter last, uint32_t samplingFrequency) const noexcept;

		// Returns an index of a sample with the highest peak
		template<typename iter>
		decltype(auto) GetPeakQuefrency(iter first, iter last, uint32_t samplingFrequency) const noexcept;

		template<typename T1, typename T2>
		float QuefrencyToFrequncy(T1 quefrency, T2 samplingFrequency) const noexcept;

		// Function takes frequency and returns PitchAnalysisResult object
		PitchAnalysisResult GetNote(float frequency) const noexcept;

		// Function performs harmonic analysis on input signal and calls the callback function (passed in PitchAnalyzer::Run()) 
		// for each analysis performed.
		void Analyze(AudioBufferIteratorPair audioBufferIters) noexcept;

		// Function used for filling the noteFrequencies std::map.
		NoteFrequenciesMap InitializeNoteFrequenciesMap() noexcept;

		// AudioInput BufferFilled event callback
		void AudioInput_BufferFilled(AudioInput& sender, AudioBufferIteratorPair args);

		AudioBufferIteratorPair GetNextAudioBufferIters();

#ifdef LOG_ANALYSIS
		// Create matlab .m file with filter parameters plots
		winrt::Windows::Foundation::IAsyncAction CreateFilterParametersLog();
#endif;
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
		const iter maxFreqIter = std::next(first, static_cast<diff_t>(1U + static_cast<uint32_t>(MAX_FREQUENCY) * N / samplingFrequency));

		WINRT_ASSERT(maxFreqIter < last);
		
		// Index of the sample representing lower frequency bound
		diff_t n = static_cast<diff_t>(MIN_FREQUENCY) * N / static_cast<diff_t>(samplingFrequency);

		std::pair<float, float> highestAmplFreq{ 0.0, 0.0 };
		std::pair<float, float> tmpAmplFreq{ 0.0, 0.0 };

		// Check only frequencies in specified range
		first += n;

		while (first != maxFreqIter) {
			tmpAmplFreq = std::make_pair(std::abs(*first), n * static_cast<diff_t>(samplingFrequency) / N);
			if (tmpAmplFreq.first > highestAmplFreq.first) {
				highestAmplFreq = tmpAmplFreq;
			}
			first++;
			n++;
		}

		return highestAmplFreq.second;
	}

	template<typename iter>
	decltype(auto) PitchAnalyzer::GetPeakQuefrency(iter first, iter last, uint32_t samplingFrequency) const noexcept
	{
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		// Number of samples
		const diff_t N = std::distance<iter>(first, last);
		// Iterator to the upper quefrency bound
		const iter maxQuefIter = std::next(first, static_cast<diff_t>(samplingFrequency / static_cast<diff_t>(MIN_FREQUENCY)));

		WINRT_ASSERT(maxQuefIter < last);

		// Index of the lower quefrency bound
		diff_t n = static_cast<diff_t>(samplingFrequency) / static_cast<diff_t>(MAX_FREQUENCY);
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

	template<typename T1, typename T2>
	inline float PitchAnalyzer::QuefrencyToFrequncy(T1 quefrency, T2 samplingFrequency) const noexcept
	{
		return static_cast<float>(samplingFrequency) / static_cast<float>(quefrency);
	}

	inline PitchAnalyzer::AudioBufferIteratorPair PitchAnalyzer::GetNextAudioBufferIters()
	{
		auto iters = audioBufferIterPairQueue.front();
		audioBufferIterPairQueue.pop();
		return iters;
	}
}