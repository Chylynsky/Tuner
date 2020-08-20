#pragma once
#include "AudioInput.h"
#include "FilterGenerator.h"
#include "TypeAliases.h"

// Enable/disable Matlab code generation
// If defined, debugging will stop on every 
// sound analysis performed
//#define CREATE_MATLAB_PLOTS

#if defined NDEBUG && defined CREATE_MATLAB_PLOTS
#undef CREATE_MATLAB_PLOTS
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
		template <uint32_t size> 
		using FFTResultBuffer		= std::array<complex_t, size>;
		using NoteFrequenciesMap	= std::map<sample_t, std::string>;
		using SoundAnalyzedCallback = std::function<void(const std::string& note, float frequency, float cents)>;

		// Struct holding the result of each, returned from GetNote() function.
		struct PitchAnalysisResult
		{
			const std::string& note;
			const float cents;
		};

		static constexpr uint32_t FILTER_SIZE{ 1 << 12 };
		static constexpr uint32_t OUTPUT_SIGNAL_SIZE{ AUDIO_BUFFER_SIZE + FILTER_SIZE - 1U };
		static constexpr uint32_t FFT_RESULT_SIZE{ OUTPUT_SIGNAL_SIZE / 2U + 1U };

		// Requested frequency range
		static constexpr float MIN_FREQUENCY{ 80.0f };
		static constexpr float MAX_FREQUENCY{ 1200.0f };
		// A4 base note frequency
		static constexpr float BASE_NOTE_FREQUENCY{ 440.0f };

		float samplingFrequency{ 44100.0f };

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

		// Function performs harmonic analysis on input signal and calls the callback function (passed in PitchAnalyzer::Run()) 
		// for each analysis performed.
		void Analyze(sample_t* first, sample_t* last) noexcept;

	private:

		// Event handlers
		SoundAnalyzedCallback soundAnalyzedCallback;

		fftwf_plan fftPlan;
		FFTResultBuffer<FFT_RESULT_SIZE> fftResult;

		// Frequencies and notes that represents them are stored in std::map<float, std::string>
		const NoteFrequenciesMap noteFrequencies{ InitializeNoteFrequenciesMap() };

		// FIR filter parameters
		SampleBuffer<OUTPUT_SIGNAL_SIZE> filterCoeff;
		FFTResultBuffer<FFT_RESULT_SIZE> filterFreqResponse;
		
		// Convolution result
		SampleBuffer<OUTPUT_SIGNAL_SIZE> outputSignal;

		template<typename iter>
		float HarmonicProductSpectrum(iter first, iter last) const noexcept;

		// Analyzes input frequency and returns a filled PitchAnalysisResult struct
		PitchAnalysisResult GetNote(float frequency) const noexcept;

		// Function used for filling the noteFrequencies std::map
		NoteFrequenciesMap InitializeNoteFrequenciesMap() const noexcept;

		// Save fftwf_plan to LocalState folder via FFTW Wisdom
		winrt::Windows::Foundation::IAsyncAction SaveFFTPlan() const noexcept;

		// Load fftwf_plan
		winrt::Windows::Foundation::IAsyncOperation<bool> LoadFFTPlan() const noexcept;

#ifdef CREATE_MATLAB_PLOTS
		// Create matlab .m file with filter parameters plots, saved in app's storage folder
		winrt::Windows::Foundation::IAsyncAction ExportFilterMatlab() const noexcept;

		// Create matlab .m file with filter parameters plots, saved in app's storage folder
		winrt::Windows::Foundation::IAsyncAction ExportSoundAnalysisMatlab(sample_t* audioBufferFirst, complex_t* fftResultFirst) const noexcept;
#endif
	};

	inline void PitchAnalyzer::SetSamplingFrequency(float samplingFrequency) noexcept
	{
		this->samplingFrequency = samplingFrequency;
	}

	inline void PitchAnalyzer::SoundAnalyzed(SoundAnalyzedCallback soundAnalyzedCallback) noexcept
	{
		WINRT_ASSERT(soundAnalyzedCallback);
		this->soundAnalyzedCallback = soundAnalyzedCallback;
	}

	template<typename iter>
	float PitchAnalyzer::HarmonicProductSpectrum(iter first, iter last) const noexcept
	{
		WINRT_ASSERT(std::distance<iter>(first, last) > 0);
		using diff_t = typename std::iterator_traits<iter>::difference_type;

		// Number of samples
		static const diff_t N{ std::distance<iter>(first, last) };
		// Iterator to the upper frequency boundary
		static const iter maxFreqIter{ std::next(first, static_cast<diff_t>(1U + static_cast<diff_t>(MAX_FREQUENCY) * N / static_cast<diff_t>(samplingFrequency))) };
		WINRT_ASSERT(maxFreqIter <= last);

		std::pair<float, float> highestSumIndex{ 0.0, 0.0 };
		std::pair<float, float> currentSumIndex{ 0.0, 0.0 };

		// Index of the sample representing lower frequency bound
		diff_t n{ static_cast<diff_t>(MIN_FREQUENCY) * N / static_cast<diff_t>(samplingFrequency) };

		for (; std::next(first, n) < maxFreqIter; n++) {
			currentSumIndex = std::make_pair(
				std::abs(*std::next(first, n)) *
				((2 * n <= N) ? std::abs(*std::next(first, 2 * n)) : 0) *
				((3 * n <= N) ? std::abs(*std::next(first, 3 * n)) : 0), n);
			if (currentSumIndex.first > highestSumIndex.first) {
				highestSumIndex = currentSumIndex;
			}
		}
		return highestSumIndex.second * static_cast<diff_t>(samplingFrequency) / N;
	}
}