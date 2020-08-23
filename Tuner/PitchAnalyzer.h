#pragma once
#include "AudioInput.h"
#include "FilterGenerator.h"

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
	template <uint32_t audioBufferSize, uint32_t filterSize, typename sample_t = float>
	class PitchAnalyzer
	{
	public:
		static constexpr uint32_t s_outputSignalSize{ audioBufferSize + filterSize - 1U };
		static constexpr uint32_t s_fftResultSize{ s_outputSignalSize / 2U + 1U };

		// Type aliases
		using complex_t				= std::complex<sample_t>;
		using SampleBuffer			= std::array<sample_t, s_outputSignalSize>;
		using WindowCoeffBuffer		= std::array<sample_t, audioBufferSize>;
		using FFTResultBuffer		= std::array<complex_t, s_fftResultSize>;
		using NoteFrequenciesMap	= std::map<sample_t, std::string>;
		using SoundAnalyzedCallback = std::function<void(const std::string& note, float frequency, float cents)>;

		PitchAnalyzer(float baseFrequency, float minFrequency, float maxFrequency, float samplingFrequency);
		~PitchAnalyzer();

		// Set sampling frequency of the audio input device. This step
		// is neccessary to make sure the calculations performed are accurate.
		// Default value is 44.1kHz.
		void SetSamplingFrequency(float samplingFrequency) noexcept;

		// Set base tone frequency.
		void SetBaseToneFrequency(float baseToneFrequency) noexcept;

		// Attach function that gets called when sound analysis is completed
		void SoundAnalyzed(SoundAnalyzedCallback soundAnalyzedCallback) noexcept;

		// Deduce the best performant FFT algorithm or, if possible, load it from file
		winrt::Windows::Foundation::IAsyncAction InitializeAsync() noexcept;

		// Function performs harmonic analysis on input signal and calls the callback function (passed in PitchAnalyzer::Run()) 
		// for each analysis performed.
		void Analyze(sample_t* first, sample_t* last) noexcept;

	private:

		// Struct holding the result of each, returned from GetNote() function.
		struct PitchAnalysisResult
		{
			const std::string& note;
			const float cents;
		};

		// Callback function called when sound is analyzed
		SoundAnalyzedCallback m_soundAnalyzedCallback;

		fftwf_plan m_pFftPlan;
		FFTResultBuffer m_fftResult;

		// FIR filter parameters
		SampleBuffer m_filterCoeff;
		FFTResultBuffer m_filterFreqResponse;

		// Window coefficients
		WindowCoeffBuffer m_windowCoeffBuffer;

		const float m_baseToneFrequency;

		// Requested frequency range
		const float m_minFrequency;
		const float m_maxFrequency;

		float m_samplingFrequency;

		// Frequencies and notes that represents them are stored in std::map<float, std::string>
		const NoteFrequenciesMap m_noteFrequencies;

		// Function used to fill the noteFrequencies NoteFrequenciesMap.
		auto InitializeNoteFrequenciesMap() noexcept;

		template<typename _InIt>
		float HarmonicProductSpectrum(_InIt first, _InIt last) const noexcept;

		// Analyzes input frequency and returns a filled PitchAnalysisResult struct
		auto GetNote(float frequency) const noexcept;

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

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline PitchAnalyzer<audioBufferSize, filterSize, sample_t>::PitchAnalyzer(float baseFrequency, float minFrequency, float maxFrequency, float samplingFrequency) 
		: m_pFftPlan{ nullptr }, m_baseToneFrequency{ baseFrequency }, m_minFrequency{ minFrequency }, 
		m_maxFrequency{ maxFrequency }, m_samplingFrequency{ samplingFrequency }, m_noteFrequencies{ std::move(InitializeNoteFrequenciesMap()) }
	{
		m_filterCoeff.fill(0.0f);
		m_windowCoeffBuffer.fill(0.0f);
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline PitchAnalyzer<audioBufferSize, filterSize, sample_t>::~PitchAnalyzer()
	{
		if (m_pFftPlan)
		{
			fftwf_destroy_plan(m_pFftPlan);
		}
		fftwf_cleanup();
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline void PitchAnalyzer<audioBufferSize, filterSize, sample_t>::SetSamplingFrequency(float newSamplingFrequency) noexcept
	{
		this->m_samplingFrequency = newSamplingFrequency;
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline void PitchAnalyzer<audioBufferSize, filterSize, sample_t>::SetBaseToneFrequency(float baseToneFrequency) noexcept
	{
		m_baseToneFrequency = baseToneFrequency;
		m_noteFrequencies = InitializeNoteFrequenciesMap();
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline void PitchAnalyzer<audioBufferSize, filterSize, sample_t>::SoundAnalyzed(SoundAnalyzedCallback callback) noexcept
	{
		// soundAnalyzedCallback should be valid at this point
		WINRT_ASSERT(callback);
		this->m_soundAnalyzedCallback = callback;
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline winrt::Windows::Foundation::IAsyncAction PitchAnalyzer<audioBufferSize, filterSize, sample_t>::InitializeAsync() noexcept
	{
		// Check if FFT plan was created earlier
		bool loadFFTResult = co_await LoadFFTPlan();

		if (!loadFFTResult) 
		{
			m_pFftPlan = fftwf_plan_dft_r2c_1d(
				s_fftResultSize,
				m_filterCoeff.data(),
				reinterpret_cast<fftwf_complex*>(m_filterFreqResponse.data()),
				FFTW_MEASURE);
			// Save created file
			co_await SaveFFTPlan();
		}
		else 
		{
			m_pFftPlan = fftwf_plan_dft_r2c_1d(
				s_fftResultSize,
				m_filterCoeff.data(),
				reinterpret_cast<fftwf_complex*>(m_filterFreqResponse.data()),
				FFTW_WISDOM_ONLY);
		}

		// FFTW plan should be valid at this point
		WINRT_ASSERT(m_pFftPlan);

		// Generate filter coefficients
		DSP::GenerateBandPassFIR(
			m_minFrequency,
			m_maxFrequency,
			m_samplingFrequency,
			m_filterCoeff.begin(),
			std::next(m_filterCoeff.begin(), filterSize),
			DSP::WindowGenerator::WindowType::BlackmanHarris);

		DSP::WindowGenerator::Generate(
			DSP::WindowGenerator::WindowType::BlackmanHarris,
			m_windowCoeffBuffer.begin(),
			m_windowCoeffBuffer.end());

		fftwf_execute(m_pFftPlan);

#ifdef CREATE_MATLAB_PLOTS
		ExportFilterMatlab();
#endif

	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline void PitchAnalyzer<audioBufferSize, filterSize, sample_t>::Analyze(sample_t* first, sample_t* last) noexcept
	{
		// SoundAnalyzed callback must be attached before performing analysis.
		WINRT_ASSERT(m_soundAnalyzedCallback);

		// Get helper pointers
		complex_t* filterFreqResponseFirst = m_filterFreqResponse.data();
		complex_t* fftResultFirst = m_fftResult.data();
		complex_t* fftResultLast = fftResultFirst + s_fftResultSize;

		// Apply window function before FFT
		std::transform(std::execution::par, first, last, m_windowCoeffBuffer.begin(), first, std::multiplies<sample_t>());

		// Execute FFT on the input signal
		fftwf_execute_dft_r2c(m_pFftPlan, first, reinterpret_cast<fftwf_complex*>(fftResultFirst));

		// Apply FIR filter to the input signal
		std::transform(std::execution::par, fftResultFirst, fftResultLast, filterFreqResponseFirst, fftResultFirst, std::multiplies<complex_t>());

#ifdef CREATE_MATLAB_PLOTS
		ExportSoundAnalysisMatlab(first, fftResultFirst).get();
		// Pause debugging, Matlab .m files are now ready
		__debugbreak();
#endif

		float firstHarmonic = HarmonicProductSpectrum(fftResultFirst, fftResultLast);

		// Check if frequency of the peak is in the requested range
		if (firstHarmonic >= m_minFrequency && firstHarmonic <= m_maxFrequency)
		{
			PitchAnalysisResult measurement{ GetNote(firstHarmonic) };
			m_soundAnalyzedCallback(measurement.note, firstHarmonic, measurement.cents);
		}
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline auto PitchAnalyzer<audioBufferSize, filterSize, sample_t>::InitializeNoteFrequenciesMap() noexcept
	{
		// Constant needed for note frequencies calculation
		const float a{ pow(2.0f, 1.0f / 12.0f) };
		const std::vector<std::string> octave{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		const auto baseNoteIter{ std::next(octave.begin(), 9) };

		auto octaveIter{ baseNoteIter };
		float currentFrequency{ m_baseToneFrequency };
		int currentOctave{ 4 };
		int halfSteps{ 0 };
		NoteFrequenciesMap result;

		// Fill a map for notes below  A4
		while (currentFrequency >= m_minFrequency) {
			currentFrequency = m_baseToneFrequency * std::pow(a, halfSteps);
			result[currentFrequency] = *octaveIter + std::to_string(currentOctave);
			halfSteps--;

			if (octaveIter == octave.begin()) {
				octaveIter = octave.end();
				currentOctave--;
			}

			octaveIter = std::prev(octaveIter, 1);
		}

		// Set iterator to one half-step above A4
		octaveIter = std::next(baseNoteIter, 1);
		halfSteps = 1;
		currentOctave = 4;

		// Fill a map for notes above and equal A4
		while (currentFrequency <= m_maxFrequency) {

			if (octaveIter == octave.end()) {
				octaveIter = octave.begin();
				currentOctave++;
			}

			currentFrequency = m_baseToneFrequency * std::pow(a, halfSteps);
			result[currentFrequency] = *octaveIter + std::to_string(currentOctave);
			halfSteps++;
			std::advance(octaveIter, 1);
		}

		return result;
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	template<typename _InIt>
	inline float PitchAnalyzer<audioBufferSize, filterSize, sample_t>::HarmonicProductSpectrum(_InIt first, _InIt last) const noexcept
	{
		using value_t = typename std::iterator_traits<_InIt>::value_type;
		using diff_t = typename std::iterator_traits<_InIt>::difference_type;

		// Number of samples
		static const diff_t N = std::distance(first, last);

		// Iterator to the upper frequency boundary
		static const _InIt maxFreqIter = std::next(first, static_cast<diff_t>(1U + static_cast<diff_t>(m_maxFrequency) * N / static_cast<diff_t>(m_samplingFrequency)));

		// Index of the sample representing lower frequency bound
		diff_t n = static_cast<diff_t>(m_minFrequency) * N / static_cast<diff_t>(m_samplingFrequency);
		auto highestSumIndex = std::make_pair(0.0f, 0.0f);
		for (; std::distance(std::next(first, 3 * n), maxFreqIter) > 0; n++) {
			auto currentSumIndex = std::make_pair(
				std::abs(*std::next(first, n)) *
				std::abs(*std::next(first, 2 * n)) *
				std::abs(*std::next(first, 3 * n)), n);
			if (currentSumIndex.first > highestSumIndex.first) {
				highestSumIndex = currentSumIndex;
			}
		}

		return highestSumIndex.second * m_samplingFrequency / static_cast<float>(N);
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline auto PitchAnalyzer<audioBufferSize, filterSize, sample_t>::GetNote(float frequency) const noexcept
	{
		// Get the nearest note above or equal
		auto high = m_noteFrequencies.lower_bound(frequency);
		// Get the nearest note below
		auto low = std::prev(high);

		// Get differences between current frequency and the nearest "valid" tones
		float highDiff = (*high).first - frequency;
		float lowDiff = frequency - (*low).first;

		if (highDiff < lowDiff)
		{
			return PitchAnalysisResult{ (*high).second, 1200.0f * std::log2(frequency / (*high).first) };
		}
		else
		{
			return PitchAnalysisResult{ (*low).second, 1200.0f * std::log2(frequency / (*low).first) };
		}
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline winrt::Windows::Foundation::IAsyncAction PitchAnalyzer<audioBufferSize, filterSize, sample_t>::SaveFFTPlan() const noexcept
	{
		using namespace winrt::Windows::Storage;

		char* fftPlanBufferRaw{ fftwf_export_wisdom_to_string() };
		hstring fftPlanBuffer{ winrt::to_hstring(fftPlanBufferRaw) };

		StorageFolder storageFolder{ ApplicationData::Current().LocalFolder() };
		StorageFile file{ co_await storageFolder.CreateFileAsync(L"fft_plan.bin", CreationCollisionOption::ReplaceExisting) };
		co_await FileIO::WriteTextAsync(file, fftPlanBuffer);
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline winrt::Windows::Foundation::IAsyncOperation<bool> PitchAnalyzer<audioBufferSize, filterSize, sample_t>::LoadFFTPlan() const noexcept
	{
		using namespace winrt::Windows::Storage;

		StorageFolder storageFolder{ ApplicationData::Current().LocalFolder() };
		IStorageItem storageItem{ co_await storageFolder.TryGetItemAsync(L"fft_plan.bin") };

		if (!storageItem) 
		{
			co_return false;
		}

		StorageFile file = storageItem.as<StorageFile>();
		std::string fftPlanBuffer{ winrt::to_string(co_await FileIO::ReadTextAsync(file)) };
		fftwf_import_wisdom_from_string(fftPlanBuffer.c_str());

		co_return true;
	}

#ifdef CREATE_MATLAB_PLOTS
	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline winrt::Windows::Foundation::IAsyncAction PitchAnalyzer<audioBufferSize, filterSize, sample_t>::ExportFilterMatlab() const noexcept
	{
		using namespace winrt::Windows::Storage;

		std::stringstream sstr;
		sstr << "fs = " << samplingFrequency << ";" << std::endl;
		sstr << "filter_size = " << FILTER_SIZE << ";" << std::endl;
		sstr << "fft_size = " << FFT_RESULT_SIZE << ";" << std::endl;
		sstr << "time_step = 1 / fs;" << std::endl;
		sstr << "freq_step = fs / fft_size;" << std::endl;
		sstr << "t = 0 : time_step : (filter_size - 1) * time_step;" << std::endl;
		sstr << "n = 0 : freq_step : fs - freq_step;" << std::endl;
		sstr << "filter = " << "[ ";
		for (auto& val : filterCoeff) {
			sstr << val << " ";
		}
		sstr << " ];" << std::endl;

		sstr << "filter_freq_response = " << "[ ";
		for (auto& val : filterFreqResponse) {
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

		StorageFolder storageFolder{ ApplicationData::Current().LocalFolder() };
		StorageFile file{ co_await storageFolder.CreateFileAsync(L"filter_log.m", CreationCollisionOption::ReplaceExisting) };
		co_await FileIO::WriteTextAsync(file, to_hstring(sstr.str()));
	}

	template<uint32_t audioBufferSize, uint32_t filterSize, typename sample_t>
	inline winrt::Windows::Foundation::IAsyncAction 
		PitchAnalyzer<audioBufferSize, filterSize, sample_t>::ExportSoundAnalysisMatlab(sample_t* audioBufferFirst, complex_t* fftResultFirst) const noexcept
	{
		using namespace winrt::Windows::Storage;

		std::stringstream sstr;
		sstr << "fs = " << samplingFrequency << ";" << std::endl;
		sstr << "input_size = " << AUDIO_BUFFER_SIZE << ";" << std::endl;
		sstr << "fft_size = " << FFT_RESULT_SIZE << ";" << std::endl;
		sstr << "time_step = 1 / fs;" << std::endl;
		sstr << "freq_step = fs / fft_size;" << std::endl;
		sstr << "t = 0 : time_step : (input_size - 1) * time_step;" << std::endl;
		sstr << "n = 0 : freq_step : fs - freq_step;" << std::endl;

		sstr << "input = " << "[ ";
		auto audioBufferLast = audioBufferFirst + AUDIO_BUFFER_SIZE;
		while (audioBufferFirst != audioBufferLast) {
			sstr << *audioBufferFirst << " ";
			audioBufferFirst++;
		}
		sstr << " ];" << std::endl;

		sstr << "spectrum = " << "[ ";
		auto fftResultLast = fftResultFirst + FFT_RESULT_SIZE;
		while (fftResultFirst != fftResultLast) {
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

		StorageFolder storageFolder{ ApplicationData::Current().LocalFolder() };
		StorageFile file{ co_await storageFolder.CreateFileAsync(L"analysis_log.m", CreationCollisionOption::ReplaceExisting) };
		co_await FileIO::WriteTextAsync(file, to_hstring(sstr.str()));
	}
#endif
}