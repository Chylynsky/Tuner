#include "pch.h"
#include "PitchAnalyzer.h"

namespace winrt::Tuner::implementation
{
	PitchAnalyzer::PitchAnalysisResult PitchAnalyzer::GetNote(float frequency) const noexcept
	{
		// Get the nearest note above or equal
		auto high = noteFrequencies.lower_bound(frequency);
		// Get the nearest note below
		auto low = std::prev(high);

		WINRT_ASSERT(*high <= *(std::prev(noteFrequencies.end())) && *high > * (noteFrequencies.begin()));

		float highDiff = (*high).first - frequency;
		float lowDiff = frequency - (*low).first;

		if (highDiff < lowDiff) {
			return { (*high).second, 1200.0f * std::log2((*high).first / frequency) };
		}
		else {
			return { (*low).second, 1200.0f * std::log2((*low).first / frequency) };
		}
	}

	void PitchAnalyzer::Analyze(AudioBufferIteratorPair audioBufferIters) noexcept
	{
		auto lock = std::lock_guard<std::mutex>(pitchAnalyzerMtx);

		// Get helper pointers
		sample_t* audioBufferFirst			= audioBufferIters.first;
		sample_t* outputFirst				= outputSignal.data();
		complex_t* filterFreqResponseFirst	= filterFreqResponse.data();
		complex_t* fftResultFirst			= fftResult.data();
		complex_t* fftResultLast			= fftResultFirst + FFT_RESULT_SIZE;

		// Execute FFT on the input signal
		fftwf_execute_dft_r2c(fftPlan, audioBufferFirst, reinterpret_cast<fftwf_complex*>(fftResultFirst));
	
		// Apply FIR filter to the input signal
		std::transform(fftResultFirst, fftResultLast, filterFreqResponseFirst, fftResultFirst,  std::multiplies<complex_t>());

#ifdef LOG_ANALYSIS
		ExportSoundAnalysisMatlab(audioBufferFirst, fftResultFirst).get();
#endif

		float firstHarmonic = GetFirstHarmonic(fftResultFirst, fftResultLast, audioInput.GetSampleRate());

		// Check if frequency of the peak is in the requested range
		if (firstHarmonic >= MIN_FREQUENCY && firstHarmonic <= MAX_FREQUENCY) {
			PitchAnalysisResult measurement{ GetNote(firstHarmonic) };
			soundAnalyzedCallback(measurement.note, firstHarmonic, measurement.cents);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		// Push analyzed buffer to the end of the queue
		audioBufferIterPairQueue.push(audioBufferIters);
	}

	PitchAnalyzer::NoteFrequenciesMap PitchAnalyzer::InitializeNoteFrequenciesMap() noexcept
	{
		// Constant needed for note frequencies calculation
		const float a{ std::pow(2.0f, 1.0f / 12.0f) };
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

	void PitchAnalyzer::AudioInput_BufferFilled(AudioInput& sender, PitchAnalyzer::AudioBufferIteratorPair args)
	{
		WINRT_ASSERT(audioBufferIterPairQueue.size() > 0);

		// Attach new buffer
		sender.AttachBuffer(GetNextAudioBufferIters());
		// Run harmonic analysis
		std::async(std::launch::async, std::bind(&PitchAnalyzer::Analyze, this, args));
	}

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

	void PitchAnalyzer::SoundAnalyzed(SoundAnalyzedCallback soundAnalyzedCallback) noexcept
	{
		this->soundAnalyzedCallback = soundAnalyzedCallback;
	}

	winrt::Windows::Foundation::IAsyncAction PitchAnalyzer::Run()
	{
		// Initialize AudioInput object
		co_await audioInput.Initialize();

		// Check if FFT plan was created earlier
		bool loadFFTResult = co_await LoadFFTPlan();
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
			audioInput.GetSampleRate(),
			filterCoeff.begin(),
			std::next(filterCoeff.begin(), FILTER_SIZE),
			DSP::WindowGenerator::WindowType::BlackmanHarris);

		fftwf_execute(fftPlan);

#ifdef LOG_ANALYSIS
		ExportFilterMatlab();
#endif

		// Get first buffer from queue and attach it to AudioInput class object
		audioInput.AttachBuffer(GetNextAudioBufferIters());
		// Pass function as callback
		audioInput.BufferFilled(std::bind(&PitchAnalyzer::AudioInput_BufferFilled, this, std::placeholders::_1, std::placeholders::_2));
		// Start recording input
		audioInput.Start();
	}

	winrt::Windows::Foundation::IAsyncAction PitchAnalyzer::SaveFFTPlan()
	{
		char* fftPlanBufferRaw = fftwf_export_wisdom_to_string();
		winrt::hstring fftPlanBuffer{ winrt::to_hstring(fftPlanBufferRaw) };

		Windows::Storage::StorageFolder storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
		Windows::Storage::StorageFile file{ co_await storageFolder.CreateFileAsync(L"fft_plan.bin", Windows::Storage::CreationCollisionOption::ReplaceExisting) };
		co_await Windows::Storage::FileIO::WriteTextAsync(file, fftPlanBuffer);
	}

	winrt::Windows::Foundation::IAsyncOperation<bool> PitchAnalyzer::LoadFFTPlan()
	{
		Windows::Storage::StorageFolder storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
		try {
			Windows::Storage::StorageFile file = co_await storageFolder.GetFileAsync(L"fft_plan.bin");
			std::string fftPlanBuffer = winrt::to_string(co_await Windows::Storage::FileIO::ReadTextAsync(file));
			fftwf_import_wisdom_from_string(fftPlanBuffer.c_str());
			co_return true;
		}
		catch (const winrt::hresult_error& e) {
			co_return false;
		}
	}

#ifdef LOG_ANALYSIS
	winrt::Windows::Foundation::IAsyncAction PitchAnalyzer::ExportFilterMatlab()
	{
		std::stringstream sstr;
		sstr << "fs = " << audioInput.GetSampleRate() << ";" << std::endl;
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
		sstr << "title('Impulse response')" << std::endl;
		sstr << "nexttile" << std::endl;
		sstr << "plot(n, filter_freq_response)" << std::endl;
		sstr << "xlabel('Frequency [Hz]')" << std::endl;
		sstr << "ylabel('Magnitude [dB]')" << std::endl;
		sstr << "title('Transfer function')" << std::endl;

		Windows::Storage::StorageFolder storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
		Windows::Storage::StorageFile file{ co_await storageFolder.CreateFileAsync(L"filter_log.m", Windows::Storage::CreationCollisionOption::ReplaceExisting) };
		co_await Windows::Storage::FileIO::WriteTextAsync(file, to_hstring(sstr.str()));
	}

	winrt::Windows::Foundation::IAsyncAction PitchAnalyzer::ExportSoundAnalysisMatlab(sample_t* audioBufferFirst, complex_t* fftResultFirst)
	{
		std::stringstream sstr;
		sstr << "fs = " << audioInput.GetSampleRate() << ";" << std::endl;
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
		sstr << "title('Input signal')" << std::endl;
		sstr << "nexttile" << std::endl;
		sstr << "plot(n, spectrum)" << std::endl;
		sstr << "xlabel('Frequency [Hz]')" << std::endl;
		sstr << "ylabel('Magnitude [dB]')" << std::endl;
		sstr << "title('Spectrum')" << std::endl;

		Windows::Storage::StorageFolder storageFolder{ Windows::Storage::ApplicationData::Current().LocalFolder() };
		Windows::Storage::StorageFile file{ co_await storageFolder.CreateFileAsync(L"analysis_log.m", Windows::Storage::CreationCollisionOption::ReplaceExisting) };
		co_await Windows::Storage::FileIO::WriteTextAsync(file, to_hstring(sstr.str()));
	}
#endif
}