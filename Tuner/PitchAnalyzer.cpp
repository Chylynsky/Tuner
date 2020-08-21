﻿#include "pch.h"
#include "PitchAnalyzer.h"

using namespace winrt;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Foundation;

namespace winrt::Tuner::implementation
{
	const PitchAnalyzer::NoteFrequenciesMap PitchAnalyzer::noteFrequencies{ InitializeNoteFrequenciesMap() };

	PitchAnalyzer::PitchAnalyzer() : fftPlan{ nullptr }, samplingFrequency{ 44100.0f }
	{
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
		auto high = noteFrequencies.lower_bound(frequency);
		// Get the nearest note below
		auto low = std::prev(high);

		// Get differences between current frequency and the nearest "valid" tones
		float highDiff = (*high).first - frequency;
		float lowDiff = frequency - (*low).first;

		//auto x = 1200.0f * std::log2((*high).first / ((*low).first + ((*high).first - (*low).first) / 2.0f));

		if (highDiff < lowDiff) 
		{
			return { (*high).second, 1200.0f * std::log2(frequency / (*high).first) };
		}
		else 
		{
			return { (*low).second, 1200.0f * std::log2(frequency / (*low).first) };
		}
	}

	PitchAnalyzer::NoteFrequenciesMap PitchAnalyzer::InitializeNoteFrequenciesMap() noexcept
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

		// Fill a map for notes below  A4
		while (currentFrequency >= MIN_FREQUENCY) {
			currentFrequency = BASE_NOTE_FREQUENCY * std::pow(a, halfSteps);
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
		while (currentFrequency <= MAX_FREQUENCY) {

			if (octaveIter == octave.end()) {
				octaveIter = octave.begin();
				currentOctave++;
			}

			currentFrequency = BASE_NOTE_FREQUENCY * std::pow(a, halfSteps);
			result[currentFrequency] = *octaveIter + std::to_string(currentOctave);
			halfSteps++;
			std::advance(octaveIter, 1);
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
				FFTW_MEASURE);
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

		// FFTW plan should be valid at this point
		WINRT_ASSERT(fftPlan);

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

	void PitchAnalyzer::Analyze(sample_t* first, sample_t* last) noexcept
	{
		// SoundAnalyzed callback must be attached before performing analysis.
		WINRT_ASSERT(soundAnalyzedCallback);

		// Get helper pointers
		complex_t* filterFreqResponseFirst	= filterFreqResponse.data();
		complex_t* fftResultFirst			= fftResult.data();
		complex_t* fftResultLast			= fftResultFirst + FFT_RESULT_SIZE;

		// Execute FFT on the input signal
		fftwf_execute_dft_r2c(fftPlan, first, reinterpret_cast<fftwf_complex*>(fftResultFirst));
		// Apply FIR filter to the input signal
		std::transform(std::execution::par, fftResultFirst, fftResultLast, filterFreqResponseFirst, fftResultFirst, std::multiplies<complex_t>());

#ifdef CREATE_MATLAB_PLOTS
		ExportSoundAnalysisMatlab(first, fftResultFirst).get();
		// Pause debugging, Matlab .m files are now ready
		__debugbreak();
#endif

		float firstHarmonic = HarmonicProductSpectrum(fftResultFirst, fftResultLast);

		// Check if frequency of the peak is in the requested range
		if (firstHarmonic >= MIN_FREQUENCY && firstHarmonic <= MAX_FREQUENCY) {
			PitchAnalysisResult measurement{ GetNote(firstHarmonic) };
			soundAnalyzedCallback(measurement.note, firstHarmonic, measurement.cents);
		}
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