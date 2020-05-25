#include "pch.h"
#include "PitchAnalyzer.h"

using namespace winrt;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Foundation;

namespace winrt::Tuner::implementation
{
	PitchAnalyzer::PitchAnalyzer() : fftPlan{ nullptr }
	{
		/*
			All vectors holding real values are resized to OUTPUT_SIGNAL_SIZE,
			which results in zero padding.
		*/

		// Add buffers to queue
		for (AudioBuffer& audioBuffer : audioBuffersArray) {
			audioBuffer.resize(OUTPUT_SIGNAL_SIZE);
			audioBufferIterPairQueue.push(std::make_pair(audioBuffer.data(), audioBuffer.data() + AUDIO_BUFFER_SIZE));
		}

		filterCoeff.resize(OUTPUT_SIGNAL_SIZE);
		outputSignal.resize(OUTPUT_SIGNAL_SIZE);
		filterFreqResponse.resize(FFT_RESULT_SIZE);
		fftResult.resize(FFT_RESULT_SIZE);
	}

	PitchAnalyzer::~PitchAnalyzer()
	{
		if (fftPlan) {
			fftwf_destroy_plan(fftPlan);
		}
		fftwf_cleanup();
	}

	PitchAnalyzer::PitchAnalysisResult PitchAnalyzer::GetNote(float frequency) const noexcept
	{
		// Get the nearest note above or equal
		auto high{ noteFrequencies.lower_bound(frequency) };
		// Get the nearest note below
		auto low{ std::prev(high) };

		WINRT_ASSERT(*high <= *(std::prev(noteFrequencies.end())) && *high > * (noteFrequencies.begin()));

		float highDiff{ (*high).first - frequency };
		float lowDiff{ frequency - (*low).first };

		if (highDiff < lowDiff) {
			return { (*high).second, 1200.0f * std::log2((*high).first / frequency) };
		}
		else {
			return { (*low).second, 1200.0f * std::log2((*low).first / frequency) };
		}
	}

	PitchAnalyzer::NoteFrequenciesMap PitchAnalyzer::InitializeNoteFrequenciesMap() const noexcept
	{
		// Constant needed for note frequencies calculation
		const float a{ pow(2.0f, 1.0f / 12.0f) };
		const std::vector<std::string> octave{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		const auto baseNoteIter{ std::next(octave.begin(), 9) };

		auto octaveIter{ baseNoteIter };
		float currentFrequency{ BASE_NOTE_FREQUENCY };
		int currentOctave{ 4 };
		int halfSteps{ 0 };
		NoteFrequenciesMap result;

		// Fill a map for notes below and equal AA
		while (currentFrequency >= MIN_FREQUENCY) {
			currentFrequency = BASE_NOTE_FREQUENCY * std::pow(a, halfSteps);
			result[currentFrequency] = *octaveIter + std::to_string(currentOctave);
			halfSteps--;

			if (octaveIter == octave.begin()) {
				octaveIter = octave.end();
				currentOctave--;
			}

			octaveIter--;
		}

		// Set iterator to one half-step above A4
		octaveIter = baseNoteIter + 1;
		halfSteps = 1;
		currentOctave = 4;

		// Fill a map for notes below and equal AA
		while (currentFrequency <= MAX_FREQUENCY) {

			if (octaveIter == octave.end()) {
				octaveIter = octave.begin();
				currentOctave++;
			}

			currentFrequency = BASE_NOTE_FREQUENCY * std::pow(a, halfSteps);
			result[currentFrequency] = *octaveIter + std::to_string(currentOctave);
			halfSteps++;
			octaveIter++;
		}

		return result;
	}

	IAsyncAction PitchAnalyzer::InitializeAsync() noexcept
	{
		// Check if FFT plan was created earlier
		bool loadFFTResult{ co_await LoadFFTPlan() };

		if (!loadFFTResult) {
			fftPlan = fftwf_plan_dft_r2c_1d(
				FFT_RESULT_SIZE,
				filterCoeff.data(),
				reinterpret_cast<fftwf_complex*>(filterFreqResponse.data()),
				FFTW_PATIENT);
			// Save created file
			co_await SaveFFTPlan();
		}
		else {
			fftPlan = fftwf_plan_dft_r2c_1d(
				FFT_RESULT_SIZE,
				filterCoeff.data(),
				reinterpret_cast<fftwf_complex*>(filterFreqResponse.data()),
				FFTW_WISDOM_ONLY);
		}

		// Generate filter coefficients
		DSP::GenerateBandPassFIR(
			MIN_FREQUENCY,
			MAX_FREQUENCY,
			samplingFrequency,
			filterCoeff.begin(),
			std::next(filterCoeff.begin(), FILTER_SIZE),
			DSP::WindowGenerator::WindowType::BlackmanHarris);

		fftwf_execute(fftPlan);

#ifdef CREATE_MATLAB_PLOTS
		ExportFilterMatlab();
#endif

	}

	void PitchAnalyzer::Analyze(const AudioBufferIteratorPair& audioBufferIters) noexcept
	{
		// SoundAnalyzed callback must be attached before performing analysis.
		WINRT_ASSERT(soundAnalyzedCallback);

		// Get helper pointers
		sample_t* audioBufferFirst = audioBufferIters.first;
		sample_t* outputFirst = outputSignal.data();
		complex_t* filterFreqResponseFirst = filterFreqResponse.data();
		complex_t* fftResultFirst = fftResult.data();
		complex_t* fftResultLast = fftResultFirst + FFT_RESULT_SIZE;

		// Execute FFT on the input signal
		fftwf_execute_dft_r2c(fftPlan, audioBufferFirst, reinterpret_cast<fftwf_complex*>(fftResultFirst));
		// Apply FIR filter to the input signal
		std::transform(std::execution::par, fftResultFirst, fftResultLast, filterFreqResponseFirst, fftResultFirst, std::multiplies<complex_t>());

#ifdef CREATE_MATLAB_PLOTS
		ExportSoundAnalysisMatlab(audioBufferFirst, fftResultFirst).get();
		// Pause debugging, Matlab .m files are now ready
		__debugbreak();
#endif

		float firstHarmonic = HarmonicProductSpectrum(fftResultFirst, fftResultLast);

		// Check if frequency of the peak is in the requested range
		if (firstHarmonic >= MIN_FREQUENCY && firstHarmonic <= MAX_FREQUENCY) {
			PitchAnalysisResult measurement{ GetNote(firstHarmonic) };
			soundAnalyzedCallback(measurement.note, firstHarmonic, measurement.cents);
		}

		// Put iterator pair back in the queue
		audioBufferIterPairQueue.push(audioBufferIters);
	}

	winrt::Windows::Foundation::IAsyncAction PitchAnalyzer::SaveFFTPlan() const noexcept
	{
		char* fftPlanBufferRaw{ fftwf_export_wisdom_to_string() };
		hstring fftPlanBuffer{ winrt::to_hstring(fftPlanBufferRaw) };

		StorageFolder storageFolder{ ApplicationData::Current().LocalFolder() };
		StorageFile file{ co_await storageFolder.CreateFileAsync(L"fft_plan.bin", CreationCollisionOption::ReplaceExisting) };
		co_await FileIO::WriteTextAsync(file, fftPlanBuffer);
	}

	IAsyncOperation<bool> PitchAnalyzer::LoadFFTPlan() const noexcept
	{
		StorageFolder storageFolder{ ApplicationData::Current().LocalFolder() };
		IStorageItem storageItem{ co_await storageFolder.TryGetItemAsync(L"fft_plan.bin") };

		if (!storageItem) {
			co_return false;
		}

		StorageFile file = storageItem.as<StorageFile>();
		std::string fftPlanBuffer{ winrt::to_string(co_await FileIO::ReadTextAsync(file)) };
		fftwf_import_wisdom_from_string(fftPlanBuffer.c_str());

		co_return true;
	}

#ifdef CREATE_MATLAB_PLOTS
	IAsyncAction PitchAnalyzer::ExportFilterMatlab() const noexcept
	{
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
		sstr << "nexttile" << std::endl;
		sstr << "plot(t, filter(1 : filter_size))" << std::endl;
		sstr << "xlabel('Time [s]')" << std::endl;
		sstr << "title('FIR filter impulse response')" << std::endl;
		sstr << "nexttile" << std::endl;
		sstr << "plot(n, filter_freq_response)" << std::endl;
		sstr << "xlabel('Frequency [Hz]')" << std::endl;
		sstr << "ylabel('Magnitude [dB]')" << std::endl;
		sstr << "title('FIR filter transfer function')" << std::endl;

		StorageFolder storageFolder{ ApplicationData::Current().LocalFolder() };
		StorageFile file{ co_await storageFolder.CreateFileAsync(L"filter_log.m", CreationCollisionOption::ReplaceExisting) };
		co_await FileIO::WriteTextAsync(file, to_hstring(sstr.str()));
	}

	IAsyncAction PitchAnalyzer::ExportSoundAnalysisMatlab(sample_t* audioBufferFirst, complex_t* fftResultFirst) const noexcept
	{
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
		sstr << "nexttile" << std::endl;
		sstr << "plot(t, input(1 : input_size))" << std::endl;
		sstr << "xlabel('Time [s]')" << std::endl;
		sstr << "title('Raw input signal')" << std::endl;
		sstr << "nexttile" << std::endl;
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