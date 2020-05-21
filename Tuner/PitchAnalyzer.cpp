#include "pch.h"
#include "PitchAnalyzer.h"

using namespace DSP;
using namespace std;
using namespace std::this_thread;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace winrt;
using namespace Windows::Storage;
using namespace Windows::Foundation;

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

	void PitchAnalyzer::AudioInput_BufferFilled(AudioInput* sender, const PitchAnalyzer::AudioBufferIteratorPair args) noexcept
	{
		WINRT_ASSERT(!audioBufferIterPairQueue.empty());
		// Attach new buffer
		sender->AttachBuffer(GetNextAudioBufferIters());
		// Run harmonic analysis
		async(launch::async, bind(&PitchAnalyzer::Analyze, this, args));
	}

	PitchAnalyzer::PitchAnalysisResult PitchAnalyzer::GetNote(float frequency) const noexcept
	{
		// Get the nearest note above or equal
		auto high = noteFrequencies.lower_bound(frequency);
		// Get the nearest note below
		auto low = prev(high);

		WINRT_ASSERT(*high <= *(prev(noteFrequencies.end())) && *high > * (noteFrequencies.begin()));

		float highDiff = (*high).first - frequency;
		float lowDiff = frequency - (*low).first;

		if (highDiff < lowDiff) {
			return { (*high).second, 1200.0f * log2((*high).first / frequency) };
		}
		else {
			return { (*low).second, 1200.0f * log2((*low).first / frequency) };
		}
	}

	void PitchAnalyzer::Analyze(AudioBufferIteratorPair audioBufferIters) noexcept
	{
		auto lock = lock_guard<std::mutex>(pitchAnalyzerMtx);

		// Get helper pointers
		sample_t* audioBufferFirst			= audioBufferIters.first;
		sample_t* outputFirst				= outputSignal.data();
		complex_t* filterFreqResponseFirst	= filterFreqResponse.data();
		complex_t* fftResultFirst			= fftResult.data();
		complex_t* fftResultLast			= fftResultFirst + FFT_RESULT_SIZE;

		// Execute FFT on the input signal
		fftwf_execute_dft_r2c(fftPlan, audioBufferFirst, reinterpret_cast<fftwf_complex*>(fftResultFirst));
		// Apply FIR filter to the input signal
		transform(fftResultFirst, fftResultLast, filterFreqResponseFirst, fftResultFirst,  multiplies<complex_t>());

#ifdef LOG_ANALYSIS
		ExportSoundAnalysisMatlab(audioBufferFirst, fftResultFirst).get();
#else
		sleep_for(500ms);
#endif

		float firstHarmonic = HarmonicProductSpectrum(fftResultFirst, fftResultLast);

		// Check if frequency of the peak is in the requested range
		if (firstHarmonic >= MIN_FREQUENCY && firstHarmonic <= MAX_FREQUENCY) {
			PitchAnalysisResult measurement{ GetNote(firstHarmonic) };
			soundAnalyzedCallback(measurement.note, firstHarmonic, measurement.cents);
		}

		// Push analyzed buffer to the end of the queue
		audioBufferIterPairQueue.push(audioBufferIters);
	}

	PitchAnalyzer::NoteFrequenciesMap PitchAnalyzer::InitializeNoteFrequenciesMap() const noexcept
	{
		// Constant needed for note frequencies calculation
		const float a{ pow(2.0f, 1.0f / 12.0f) };
		const array<string, 12> octave{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		const auto baseNoteIter{ next(octave.begin(), 9) };

		auto octaveIter{ baseNoteIter };
		float currentFrequency{ BASE_NOTE_FREQUENCY };
		int currentOctave{ 4 };
		int halfSteps{ 0 };
		NoteFrequenciesMap result;

		// Fill a map for notes below and equal AA
		while (currentFrequency >= MIN_FREQUENCY) {
			currentFrequency = BASE_NOTE_FREQUENCY * pow(a, halfSteps);
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

			currentFrequency = BASE_NOTE_FREQUENCY * pow(a, halfSteps);
			result[currentFrequency] = *octaveIter + std::to_string(currentOctave);
			halfSteps++;
			octaveIter++;
		}

		return result;
	}

	void PitchAnalyzer::SoundAnalyzed(SoundAnalyzedCallback soundAnalyzedCallback) noexcept
	{
		this->soundAnalyzedCallback = soundAnalyzedCallback;
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
		GenerateBandPassFIR(
			MIN_FREQUENCY,
			MAX_FREQUENCY,
			samplingFrequency,
			filterCoeff.begin(),
			std::next(filterCoeff.begin(), FILTER_SIZE),
			WindowGenerator::WindowType::BlackmanHarris);

		fftwf_execute(fftPlan);

#ifdef LOG_ANALYSIS
		ExportFilterMatlab();
#endif
	}

	winrt::Windows::Foundation::IAsyncAction PitchAnalyzer::SaveFFTPlan() const noexcept
	{
		char* fftPlanBufferRaw{ fftwf_export_wisdom_to_string() };
		hstring fftPlanBuffer{ winrt::to_hstring(fftPlanBufferRaw) };
		//std::free(fftPlanBufferRaw);

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
		string fftPlanBuffer{ winrt::to_string(co_await FileIO::ReadTextAsync(file)) };
		fftwf_import_wisdom_from_string(fftPlanBuffer.c_str());

		co_return true;
	}

#ifdef LOG_ANALYSIS
	IAsyncAction PitchAnalyzer::ExportFilterMatlab() const noexcept
	{
		stringstream sstr;
		sstr << "fs = " << samplingFrequency << ";" << endl;
		sstr << "filter_size = " << FILTER_SIZE << ";" << endl;
		sstr << "fft_size = " << FFT_RESULT_SIZE << ";" << endl;
		sstr << "time_step = 1 / fs;" << endl;
		sstr << "freq_step = fs / fft_size;" << endl;
		sstr << "t = 0 : time_step : (filter_size - 1) * time_step;" << endl;
		sstr << "n = 0 : freq_step : fs - freq_step;" << endl;
		sstr << "filter = " << "[ ";
		for (auto& val : filterCoeff) {
			sstr << val << " ";
		}
		sstr << " ];" << endl;

		sstr << "filter_freq_response = " << "[ ";
		for (auto& val : filterFreqResponse) {
			sstr << 20.0f * log10(abs(val)) << " ";
		}
		sstr << " ];" << endl;
		sstr << "nexttile" << endl;
		sstr << "plot(t, filter(1 : filter_size))" << endl;
		sstr << "xlabel('Time [s]')" << endl;
		sstr << "title('Impulse response')" << endl;
		sstr << "nexttile" << endl;
		sstr << "plot(n, filter_freq_response)" << endl;
		sstr << "xlabel('Frequency [Hz]')" << endl;
		sstr << "ylabel('Magnitude [dB]')" << endl;
		sstr << "title('Transfer function')" << endl;

		StorageFolder storageFolder{ ApplicationData::Current().LocalFolder() };
		StorageFile file{ co_await storageFolder.CreateFileAsync(L"filter_log.m", CreationCollisionOption::ReplaceExisting) };
		co_await FileIO::WriteTextAsync(file, to_hstring(sstr.str()));
	}

	IAsyncAction PitchAnalyzer::ExportSoundAnalysisMatlab(sample_t* audioBufferFirst, complex_t* fftResultFirst) const noexcept
	{
		stringstream sstr;
		sstr << "fs = " << samplingFrequency << ";" << endl;
		sstr << "input_size = " << AUDIO_BUFFER_SIZE << ";" << endl;
		sstr << "fft_size = " << FFT_RESULT_SIZE << ";" << endl;
		sstr << "time_step = 1 / fs;" << endl;
		sstr << "freq_step = fs / fft_size;" << endl;
		sstr << "t = 0 : time_step : (input_size - 1) * time_step;" << endl;
		sstr << "n = 0 : freq_step : fs - freq_step;" << endl;

		sstr << "input = " << "[ ";
		auto audioBufferLast = audioBufferFirst + AUDIO_BUFFER_SIZE;
		while (audioBufferFirst != audioBufferLast) {
			sstr << *audioBufferFirst << " ";
			audioBufferFirst++;
		}
		sstr << " ];" << endl;

		sstr << "spectrum = " << "[ ";
		auto fftResultLast = fftResultFirst + FFT_RESULT_SIZE;
		while (fftResultFirst != fftResultLast) {
			sstr << 20.0f * log10(abs(*fftResultFirst)) << " ";
			fftResultFirst++;
		}

		sstr << " ];" << endl;
		sstr << "nexttile" << endl;
		sstr << "plot(t, input(1 : input_size))" << endl;
		sstr << "xlabel('Time [s]')" << endl;
		sstr << "title('Input signal')" << endl;
		sstr << "nexttile" << endl;
		sstr << "plot(n, spectrum)" << endl;
		sstr << "xlabel('Frequency [Hz]')" << endl;
		sstr << "ylabel('Magnitude [dB]')" << endl;
		sstr << "title('Spectrum')" << endl;

		StorageFolder storageFolder{ ApplicationData::Current().LocalFolder() };
		StorageFile file{ co_await storageFolder.CreateFileAsync(L"analysis_log.m", CreationCollisionOption::ReplaceExisting) };
		co_await FileIO::WriteTextAsync(file, to_hstring(sstr.str()));
	}
#endif
}