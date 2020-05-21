#pragma once
#include "AudioInput.h"
#include "FilterGenerator.h"

//#define LOG_ANALYSIS
#if defined NDEBUG && defined LOG_ANALYSIS
#undef LOG_ANALYSIS
#endif
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
		// Type aliases
		using sample_t					= float;
		using complex_t					= std::complex<sample_t>;
		using AudioBuffer				= std::vector<sample_t>;
		using AudioBufferIteratorPair	= std::pair<sample_t*, sample_t*>;
		using FFTResultBuffer			= std::vector<complex_t>;
		using NoteFrequenciesMap		= std::map<sample_t, std::string>;
		using AudioBufferArray			= std::array<AudioBuffer, 4>;
		using AudioBufferQueue			= std::queue<AudioBufferIteratorPair>;
		using SoundAnalyzedCallback		= std::function<void(const std::string& note, float frequency, float cents)>;

		// Struct holding the result of each, returned from GetNote() function.
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
		static constexpr float MIN_FREQUENCY{ 60.0f };
		static constexpr float MAX_FREQUENCY{ 1800.0f };
		// A4 base note frequency
		static constexpr float BASE_NOTE_FREQUENCY{ 440.0f };

		float samplingFrequency{ 48000.0f };

		PitchAnalyzer();
		~PitchAnalyzer();

		// Set sampling frequency of the audio input device. This step
		// is neccessary to make sure the calculations performed are accurate.
		// Default value is 44.1kHz.
		void SetSamplingFrequency(float samplingFrequency) noexcept;

		// Attach function that gets called when sound analysis is completed
		void SoundAnalyzed(SoundAnalyzedCallback soundAnalyzedCallback) noexcept;

		// Deduce the best performant FFT algorithm or, if possible, load it from file
		winrt::Windows::Foundation::IAsyncAction InitializeAsync() noexcept;

		// Returns std::pair of pointers to available audio buffer <first; last)
		AudioBufferIteratorPair GetNextAudioBufferIters() noexcept;

		// AudioInput BufferFilled event callback
		void AudioInput_BufferFilled(AudioInput* sender, const AudioBufferIteratorPair args) noexcept;

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
		
		// Array of buffers that hold recorded samples
		AudioBufferArray audioBuffersArray;
		// Queue of available buffers
		AudioBufferQueue audioBufferIterPairQueue;

		template<typename iter>
		float HarmonicProductSpectrum(iter first, iter last) const noexcept;

		// Analyzes input frequency and returns a filled PitchAnalysisResult struct
		PitchAnalysisResult GetNote(float frequency) const noexcept;

		// Function performs harmonic analysis on input signal and calls the callback function (passed in PitchAnalyzer::Run()) 
		// for each analysis performed.
		void Analyze(AudioBufferIteratorPair audioBufferIters) noexcept;

		// Function used for filling the noteFrequencies std::map
		NoteFrequenciesMap InitializeNoteFrequenciesMap() const noexcept;

		// Save fftwf_plan to LocalState folder via FFTW Wisdom
		winrt::Windows::Foundation::IAsyncAction SaveFFTPlan() const noexcept;

		// Load fftwf_plan
		winrt::Windows::Foundation::IAsyncOperation<bool> LoadFFTPlan() const noexcept;

#ifdef LOG_ANALYSIS
		// Create matlab .m file with filter parameters plots, saved in app's storage folder
		winrt::Windows::Foundation::IAsyncAction ExportFilterMatlab() const noexcept;

		// Create matlab .m file with filter parameters plots, saved in app's storage folder
		winrt::Windows::Foundation::IAsyncAction ExportSoundAnalysisMatlab(sample_t* audioBufferFirst, complex_t* fftResultFirst) const noexcept;
#endif;
	};

	inline void PitchAnalyzer::SetSamplingFrequency(float samplingFrequency) noexcept
	{
		this->samplingFrequency = samplingFrequency;
	}

	inline PitchAnalyzer::AudioBufferIteratorPair PitchAnalyzer::GetNextAudioBufferIters() noexcept
	{
		WINRT_ASSERT(!audioBufferIterPairQueue.empty());

		auto iters = audioBufferIterPairQueue.front();
		audioBufferIterPairQueue.pop();
		return iters;
	}

	template<typename iter>
	inline float PitchAnalyzer::HarmonicProductSpectrum(iter first, iter last) const noexcept
	{
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		// Number of samples
		const diff_t N = std::distance<iter>(first, last);
		// Iterator to the upper frequency boundary
		const iter maxFreqIter{ std::next(first, static_cast<diff_t>(1U + static_cast<diff_t>(MAX_FREQUENCY) * N / static_cast<diff_t>(samplingFrequency))) };

		WINRT_ASSERT(maxFreqIter <= last);

		std::pair<float, float> highestSumIndex{ 0.0, 0.0 };
		std::pair<float, float> currentSumIndex{ 0.0, 0.0 };

		// Index of the sample representing lower frequency bound
		diff_t n{ static_cast<diff_t>(MIN_FREQUENCY) * N / static_cast<diff_t>(samplingFrequency) };

		for (; std::next(first, 5 * n) < maxFreqIter; n++) {
			currentSumIndex = std::make_pair(
				std::abs(*std::next(first, n)) *
				std::abs(*std::next(first, 2 * n)) *
				std::abs(*std::next(first, 3 * n)) *
				std::abs(*std::next(first, 4 * n)) *
				std::abs(*std::next(first, 5 * n)), n);
			if (currentSumIndex.first > highestSumIndex.first) {
				highestSumIndex = currentSumIndex;
			}
		}
		return highestSumIndex.second * static_cast<diff_t>(samplingFrequency) / N;
	}
}