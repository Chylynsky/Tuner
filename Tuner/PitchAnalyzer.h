#pragma once
#include "PitchAnalyzerTraits.h"
#include "FilterGenerator.h"
#include "DSPMath.h"
#include "FFTPlan.h"

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
	template<size_t s_audioBufferSize, size_t s_filterSize, typename sample_t = float>
	class PitchAnalyzer
	{
		static_assert(Is_positive_power_of_2(s_audioBufferSize), "Audio buffer size must be a power of 2.");
		static_assert(Is_positive_power_of_2(s_filterSize), "Filter size must be a power of 2.");

		static constexpr size_t s_filteredSignalSize	= s_audioBufferSize + s_filterSize - 1U;
		static constexpr size_t s_fftResultSize			= s_filteredSignalSize / 2U + 1U;

		// Type aliases
		using complex_t				= std::complex<sample_t>;
		using SampleBuffer			= std::array<sample_t, s_filteredSignalSize>;
		using WindowCoeffBuffer		= std::array<sample_t, s_audioBufferSize>;
		using FFTResultBuffer		= std::array<complex_t, s_fftResultSize>;
		using NoteFrequenciesMap	= std::map<sample_t, std::string>;
		using SoundAnalyzedCallback = std::function<void(const std::string& note, float frequency, float cents)>;

		// Struct holding the result of each, returned from GetNote() function.
		struct PitchAnalysisResult
		{
			const std::string& note;
			const float cents;
		};

		// Callback function called when sound is analyzed
		SoundAnalyzedCallback	m_soundAnalyzedCallback;

		FFTResultBuffer			m_fftResult;

		// FIR filter parameters
		SampleBuffer			m_filterCoeff;
		FFTResultBuffer			m_filterFreqResponse;

		// Window coefficients
		WindowCoeffBuffer		m_windowCoeffBuffer;

		// Requested frequency range
		const float				m_minFrequency;
		const float				m_maxFrequency;

		// Frequency of the base tone
		float					m_baseToneFrequency;

		// Sampling frequency
		float					m_samplingFrequency;

		DSP::FFTPlan<sample_t>	m_fftPlan;

		// Frequencies and notes that represents them are stored in std::map<float, std::string>
		NoteFrequenciesMap		m_noteFrequenciesMap;

		bool					m_initialized;

	public:

		PitchAnalyzer(float minFrequency, float maxFrequency, float baseToneFrequency = 0.0f, float samplingFrequency = 0.0f) :
			m_baseToneFrequency	{ baseToneFrequency }, 
			m_minFrequency		{ minFrequency }, 
			m_maxFrequency		{ maxFrequency }, 
			m_initialized		{ false }
		{
			// Allow for initializing values of sampling frequency and base note frequency later
			
			if (samplingFrequency > 0.0f)
			{
				SetSamplingFrequency(samplingFrequency);
			}

			if (baseToneFrequency > 0.0f)
			{
				SetBaseToneFrequency(baseToneFrequency);
			}

			// Pad arrays with zeros
			m_filterCoeff.fill(0.0f);
			m_windowCoeffBuffer.fill(0.0f);
		}

		PitchAnalyzer()						= delete;
		PitchAnalyzer(PitchAnalyzer&&)		= delete;
		PitchAnalyzer(const PitchAnalyzer&) = delete;

		~PitchAnalyzer() = default;

		PitchAnalyzer& operator=(PitchAnalyzer&&)		= delete;
		PitchAnalyzer& operator=(const PitchAnalyzer&)	= delete;
		
		// Set sampling frequency of the audio input device. This step
		// is neccessary to make sure the calculations performed are accurate.
		void SetSamplingFrequency(float samplingFrequency)
		{
			if (samplingFrequency > 0.0f)
			{
				m_samplingFrequency = samplingFrequency;
			}
			else
			{
				// Sampling frequency must be positive
				WINRT_ASSERT(0);
			}
		}
		
		// Set base tone frequency.
		void SetBaseToneFrequency(float baseToneFrequency) noexcept
		{
			if (baseToneFrequency > 0.0f)
			{
				m_baseToneFrequency = baseToneFrequency;
				m_noteFrequenciesMap = std::move(InitializeNoteFrequenciesMap());
			}
			else
			{
				// Base tone frequency must be positive
				WINRT_ASSERT(0);
			}
		}

		void SetMinFrequency(float minFrequency) noexcept
		{
			if (minFrequency >= 0.0f)
			{
				m_minFrequency = minFrequency;
				
				if (m_baseToneFrequency > 0.0f)
				{
					m_noteFrequenciesMap = std::move(InitializeNoteFrequenciesMap());
				}

				if (m_samplingFrequency > 0.0f)
				{
					GenerateNewFilter();
				}
			}
			else
			{
				// Bad value
				WINRT_ASSERT(0);
			}
		}

		void SetMaxFrequency(float maxFrequency) noexcept
		{
			if (maxFrequency >= 0.0f)
			{
				m_maxFrequency = maxFrequency;
				m_noteFrequenciesMap = std::move(InitializeNoteFrequenciesMap());
				
				if (m_baseToneFrequency > 0.0f)
				{
					m_noteFrequenciesMap = std::move(InitializeNoteFrequenciesMap());
				}

				if (m_samplingFrequency > 0.0f)
				{
					GenerateNewFilter();
				}
			}
			else
			{
				// Bad value
				WINRT_ASSERT(0);
			}
		}

		void SetFrequencyRange(float minFrequency, float maxFrequency)
		{
			if (minFrequency >= 0.0f && maxFrequency > 0.0f && maxFrequency > minFrequency)
			{
				m_minFrequency = minFrequency;
				m_maxFrequency = maxFrequency;

				if (m_baseToneFrequency > 0.0f)
				{
					m_noteFrequenciesMap = std::move(InitializeNoteFrequenciesMap());
				}

				if (m_samplingFrequency > 0.0f)
				{
					GenerateNewFilter();
				}
			}
			else
			{
				// Bad values
				WINRT_ASSERT(0);
			}
		}
		
		// Attach function that gets called when sound analysis is completed
		void SoundAnalyzed(SoundAnalyzedCallback soundAnalyzedCallback) noexcept
		{
			WINRT_ASSERT(soundAnalyzedCallback);
			m_soundAnalyzedCallback = soundAnalyzedCallback;
		}

		// Deduce the best performant FFT algorithm or, if possible, load it from file
		winrt::Windows::Foundation::IAsyncAction InitializeAsync()
		{
			// Sampling frequency and base tone frequency must be set before initialization
			WINRT_ASSERT(m_samplingFrequency > 0.0f);
			WINRT_ASSERT(m_baseToneFrequency > 0.0f);
			WINRT_ASSERT(!m_noteFrequenciesMap.empty());

			if (!m_initialized)
			{
				// Check if FFT plan was created earlier
				bool loadFFTResult = co_await DSP::FFTPlan<sample_t>::LoadFFTPlan(L"fft_plan.bin");

				auto InitializeFFTPlan = [this](DSP::flags _flags) {
					return DSP::FFTPlan<sample_t>(m_filterCoeff.begin(), m_filterCoeff.end(), m_filterFreqResponse.begin(), _flags);
				};

				m_fftPlan = InitializeFFTPlan(DSP::flags::wisdom);

				if (!m_fftPlan)
				{
					m_fftPlan = InitializeFFTPlan(DSP::flags::measure);
					m_fftPlan.SaveFFTPlan(L"fft_plan.bin");
				}
			}

			// FFT plan should be valid at this point
			WINRT_ASSERT(m_fftPlan);

			// Generate filter coefficients
			DSP::GenerateBandPassFIR(
				m_minFrequency,
				m_maxFrequency,
				m_samplingFrequency,
				m_filterCoeff.begin(),
				std::next(m_filterCoeff.begin(), s_filterSize),
				DSP::WindowGenerator::WindowType::BlackmanHarris);

			// Generate window coefficients
			DSP::WindowGenerator::Generate(
				DSP::WindowGenerator::WindowType::BlackmanHarris,
				m_windowCoeffBuffer.begin(),
				m_windowCoeffBuffer.end());

			// Execute FFT for generated filter
			m_fftPlan.Execute();
			m_initialized = true;

#ifdef CREATE_MATLAB_PLOTS
			ExportFilterMatlab();
#endif
		}

		// Function performs harmonic analysis on input signal and calls the callback function
		// for each analysis performed.
		template<typename _FwdIt>
		void Analyze(_FwdIt first, _FwdIt last) noexcept
		{
			// Object must be properly initialized
			WINRT_ASSERT(m_initialized);
			// SoundAnalyzed callback must be attached before performing analysis.
			WINRT_ASSERT(m_soundAnalyzedCallback);

			// Get helper iterators
			auto filterFreqResponseFirst	= m_filterFreqResponse.begin();
			auto fftResultFirst				= m_fftResult.begin();
			auto fftResultLast				= std::next(fftResultFirst, s_fftResultSize);
			auto windowCoeffBufferFirst		= m_windowCoeffBuffer.begin();

			// Apply window function before FFT
			DSP::MultiplyPointwise(first, last, windowCoeffBufferFirst, first);

			// Execute FFT on the input signal
			m_fftPlan.Execute(first, last, fftResultFirst);

			// Apply FIR filter to the input signal
			DSP::MultiplyPointwise(fftResultFirst, fftResultLast, filterFreqResponseFirst, fftResultFirst);

#ifdef CREATE_MATLAB_PLOTS
			ExportSoundAnalysisMatlab(first, fftResultFirst).get();
			// Pause debugging, Matlab .m files are now ready
			__debugbreak();
#endif

			float firstHarmonic = HarmonicProductSpectrum(fftResultFirst, fftResultLast);

			// Check if frequency of the peak is in the requested range
			if (firstHarmonic >= m_minFrequency && firstHarmonic <= m_maxFrequency)
			{
				PitchAnalysisResult measurement = GetNote(firstHarmonic);
				m_soundAnalyzedCallback(measurement.note, firstHarmonic, measurement.cents);
			}
		}

	private:

		// Function used to fill the noteFrequencies NoteFrequenciesMap.
		NoteFrequenciesMap InitializeNoteFrequenciesMap() noexcept
		{
			constexpr std::array<const char*, 12> octave{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

			// Constant needed for note frequencies calculation
			const float a			= pow(2.0f, 1.0f / 12.0f);
			const auto baseNoteIter	= std::next(octave.begin(), 9);

			auto octaveIter			= baseNoteIter;
			float currentFrequency	= m_baseToneFrequency;
			int currentOctave		= 4;
			int halfSteps			= 0;

			NoteFrequenciesMap result;

			// Fill a map for notes below  A4
			while (currentFrequency >= m_minFrequency)
			{
				currentFrequency = m_baseToneFrequency * std::pow(a, halfSteps);
				result[currentFrequency] = std::string(*octaveIter) + std::to_string(currentOctave);
				halfSteps--;

				if (octaveIter == octave.begin())
				{
					octaveIter = octave.end();
					currentOctave--;
				}

				octaveIter = std::prev(octaveIter, 1);
			}

			// Set iterator to one half-step above A4
			octaveIter		= std::next(baseNoteIter, 1);
			halfSteps		= 1;
			currentOctave	= 4;

			// Fill a map for notes above and equal A4
			while (currentFrequency <= m_maxFrequency) {

				if (octaveIter == octave.end())
				{
					octaveIter = octave.begin();
					currentOctave++;
				}

				currentFrequency = m_baseToneFrequency * std::pow(a, halfSteps);
				result[currentFrequency] = std::string(*octaveIter) + std::to_string(currentOctave);
				halfSteps++;
				std::advance(octaveIter, 1);
			}

			return result;
		}

		// Analyze FFT result and find the base tone frequency
		template<typename _InIt>
		float HarmonicProductSpectrum(_InIt first, _InIt last) const noexcept
		{
			using value_t	= typename std::iterator_traits<_InIt>::value_type;
			using diff_t	= typename std::iterator_traits<_InIt>::difference_type;

			// Number of samples
			static const diff_t N = std::distance(first, last);

			// Iterator to the upper frequency boundary
			static const _InIt maxFreqIter = std::next(first, static_cast<diff_t>(1U + static_cast<diff_t>(m_maxFrequency) * N / static_cast<diff_t>(m_samplingFrequency)));

			// Index of the sample representing lower frequency bound
			diff_t n = static_cast<diff_t>(m_minFrequency) * N / static_cast<diff_t>(m_samplingFrequency);
			auto highestSumIndex = std::make_pair(0.0f, 0.0f);

			for (; std::distance(std::next(first, 3 * n), maxFreqIter) > 0; n++)
			{
				auto currentSumIndex = std::make_pair(
					std::abs(*std::next(first, n)) *
					std::abs(*std::next(first, 2 * n)) *
					std::abs(*std::next(first, 3 * n)), n);

				if (currentSumIndex.first > highestSumIndex.first)
				{
					highestSumIndex = currentSumIndex;
				}
			}

			return highestSumIndex.second * m_samplingFrequency / static_cast<float>(N);
		}

		// Analyzes input frequency and returns a filled PitchAnalysisResult struct
		PitchAnalysisResult GetNote(float frequency) const noexcept
		{
			// Get the nearest note above or equal
			auto high = m_noteFrequenciesMap.lower_bound(frequency);
			// Get the nearest note below
			auto low = std::prev(high);

			// Get differences between current frequency and the nearest "valid" tones
			float highDiff = (*high).first - frequency;
			float lowDiff = frequency - (*low).first;

			if (highDiff < lowDiff)
			{
				return { (*high).second, 1200.0f * std::log2(frequency / (*high).first) };
			}
			else
			{
				return { (*low).second, 1200.0f * std::log2(frequency / (*low).first) };
			}
		}

		void GenerateNewFilter()
		{
			// Generate filter coefficients
			DSP::GenerateBandPassFIR(
				m_minFrequency,
				m_maxFrequency,
				m_samplingFrequency,
				m_filterCoeff.begin(),
				std::next(m_filterCoeff.begin(), s_filterSize),
				DSP::WindowGenerator::WindowType::BlackmanHarris);

			m_fftPlan.Execute(m_filterCoeff.begin(), m_filterCoeff.end(), m_filterFreqResponse.begin());
		}

#ifdef CREATE_MATLAB_PLOTS
		// Create matlab .m file with filter parameters plots, saved in app's storage folder
		winrt::Windows::Foundation::IAsyncAction ExportFilterMatlab() const noexcept
		{
			using namespace winrt::Windows::Storage;

			std::stringstream sstr;
			sstr << "fs = " << m_samplingFrequency << ";" << std::endl;
			sstr << "filter_size = " << s_filterSize << ";" << std::endl;
			sstr << "fft_size = " << s_fftResultSize << ";" << std::endl;
			sstr << "time_step = 1 / fs;" << std::endl;
			sstr << "freq_step = fs / fft_size;" << std::endl;
			sstr << "t = 0 : time_step : (filter_size - 1) * time_step;" << std::endl;
			sstr << "n = 0 : freq_step : fs - freq_step;" << std::endl;
			sstr << "filter = " << "[ ";
			for (auto& val : m_filterCoeff) {
				sstr << val << " ";
			}
			sstr << " ];" << std::endl;

			sstr << "filter_freq_response = " << "[ ";
			for (auto& val : m_filterFreqResponse) {
				sstr << 20.0f * std::log10(std::abs(val)) << " ";
			}
			sstr << " ];" << std::endl;
			sstr << "subplot(2, 1, 1)" << std::endl;
			sstr << "plot(t, filter(1 : filter_size))" << std::endl;
			sstr << "xlabel('Time [s]')" << std::endl;
			sstr << "title('FIR filter impulse response')" << std::endl;
			sstr << "subplot(2, 1, 1)" << std::endl;
			sstr << "plot(n, filter_freq_response)" << std::endl;
			sstr << "xlabel('Frequency [Hz]')" << std::endl;
			sstr << "ylabel('Magnitude [dB]')" << std::endl;
			sstr << "title('FIR filter transfer function')" << std::endl;

			StorageFolder storageFolder = ApplicationData::Current().LocalFolder();
			StorageFile file = co_await storageFolder.CreateFileAsync(L"filter_log.m", CreationCollisionOption::ReplaceExisting);
			co_await FileIO::WriteTextAsync(file, to_hstring(sstr.str()));
	}

		// Create matlab .m file with filter parameters plots, saved in app's storage folder
		winrt::Windows::Foundation::IAsyncAction ExportSoundAnalysisMatlab(sample_t* audioBufferFirst, complex_t* fftResultFirst) const noexcept
		{
			using namespace winrt::Windows::Storage;

			std::stringstream sstr;
			sstr << "fs = " << m_samplingFrequency << ";" << std::endl;
			sstr << "input_size = " << s_audioBufferSize << ";" << std::endl;
			sstr << "fft_size = " << s_fftResultSize << ";" << std::endl;
			sstr << "time_step = 1 / fs;" << std::endl;
			sstr << "freq_step = fs / fft_size;" << std::endl;
			sstr << "t = 0 : time_step : (input_size - 1) * time_step;" << std::endl;
			sstr << "n = 0 : freq_step : fs - freq_step;" << std::endl;

			sstr << "input = " << "[ ";
			auto audioBufferLast = audioBufferFirst + s_audioBufferSize;
			while (audioBufferFirst != audioBufferLast)
			{
				sstr << *audioBufferFirst << " ";
				audioBufferFirst++;
			}
			sstr << " ];" << std::endl;

			sstr << "spectrum = " << "[ ";
			auto fftResultLast = fftResultFirst + s_fftResultSize;
			while (fftResultFirst != fftResultLast)
			{
				sstr << 20.0f * std::log10(std::abs(*fftResultFirst)) << " ";
				fftResultFirst++;
			}

			sstr << " ];" << std::endl;
			sstr << "subplot(2, 1, 1)" << std::endl;
			sstr << "plot(t, input(1 : input_size))" << std::endl;
			sstr << "xlabel('Time [s]')" << std::endl;
			sstr << "title('Windowed input signal')" << std::endl;
			sstr << "subplot(2, 1, 2)" << std::endl;
			sstr << "plot(n, spectrum)" << std::endl;
			sstr << "xlabel('Frequency [Hz]')" << std::endl;
			sstr << "ylabel('Magnitude [dB]')" << std::endl;
			sstr << "title('Filtered input signal amplitude spectrum')" << std::endl;

			StorageFolder storageFolder = ApplicationData::Current().LocalFolder();
			StorageFile file = co_await storageFolder.CreateFileAsync(L"analysis_log.m", CreationCollisionOption::ReplaceExisting);
			co_await FileIO::WriteTextAsync(file, to_hstring(sstr.str()));
		}
#endif
	};
}