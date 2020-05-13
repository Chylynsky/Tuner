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

	void PitchAnalyzer::Analyze(PitchAnalysisBuffer* pitchAnalysisBuffer) noexcept
	{
		std::vector<double> filter(120);
		DSP::GenerateBandpassFIR(100, 600, 48000, filter.begin(), filter.end(), DSP::WindowGenerator::WindowType::Blackman);

		// auto lock = pitchAnalysisBuffer->LockBuffer();
		// Get helper pointers
		size_t audioBufferSize				= pitchAnalysisBuffer->audioBuffer.size();
		float* audioBufferFirst				= pitchAnalysisBuffer->audioBuffer.data();
		float* audioBufferLast				= audioBufferFirst + audioBufferSize;
		size_t fftResultSize				= pitchAnalysisBuffer->fftResult.size();
		std::complex<float>* fftResultFirst = pitchAnalysisBuffer->fftResult.data();
		std::complex<float>* fftResultLast	= fftResultFirst + fftResultSize;

		// Apply the window function
		DSP::WindowGenerator::ApplyWindow(audioBufferFirst, audioBufferLast, windowCoefficients.begin(), 1);

		pitchAnalysisBuffer->ExecuteFFT();

		// Prepare the FFT result for cepstrum calculation
		for (size_t i = 0; i < fftResultSize; i++) {
			audioBufferFirst[i] = audioBufferFirst[fftResultSize + i] = std::log(std::pow(std::abs(fftResultFirst[i]), 2));
		}

		// Perform a second Fourier Transform
		pitchAnalysisBuffer->ExecuteFFT();

		uint32_t sampleRate		= audioInput.GetSampleRate();
		uint32_t peakQuefrency	= static_cast<uint32_t>(GetPeakQuefrency(fftResultFirst, fftResultLast, sampleRate));
		float firstHarmonic		= QuefrencyToFrequncy(peakQuefrency, sampleRate);

		if (firstHarmonic >= minFrequency && firstHarmonic <= maxFrequency) {
			PitchAnalysisResult measurement{ GetNote(firstHarmonic) };
			soundAnalyzedCallback(measurement.note, measurement.cents, firstHarmonic);
		}
	}

	std::map<float, std::string> PitchAnalyzer::InitializeNoteFrequenciesMap() noexcept
	{
		// Constant needed for note frequencies calculation
		const float a{ std::pow(2.0f, 1.0f / 12.0f) };
		const std::vector<std::string> octave{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		const auto baseNoteIter{ std::next(octave.begin(), 9) };

		auto octaveIter{ baseNoteIter };
		float currentFrequency{ baseNoteFrequency };
		int currentOctave{ 4 };
		int halfSteps{ 0 };
		std::map<float, std::string> result;

		// Fill a map for notes below and equal AA
		while (currentFrequency >= minFrequency) {
			currentFrequency = baseNoteFrequency * std::pow(a, halfSteps);
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
		while (currentFrequency <= maxFrequency) {

			if (octaveIter == octave.end()) {
				octaveIter = octave.begin();
				currentOctave++;
			}

			currentFrequency = baseNoteFrequency * std::pow(a, halfSteps);
			result[currentFrequency] = *octaveIter + std::to_string(currentOctave);
			halfSteps++;
			octaveIter++;
		}

		return result;
	}

	void PitchAnalyzer::AudioInput_BufferFilled(AudioInput& sender, PitchAnalysisBuffer* args)
	{
		// Attach new buffer
		sender.AttachBuffer(pitchAnalysisBufferQueue.front());
		pitchAnalysisBufferQueue.pop();
		// Run harmonic analysis
		Analyze(args);
		// Push analyzed buffer to the end of the queue
		pitchAnalysisBufferQueue.push(args);
	}

	PitchAnalyzer::PitchAnalyzer(float A4, float minFrequency, float maxFrequency) :
		minFrequency{ minFrequency },
		maxFrequency{ maxFrequency },
		baseNoteFrequency{ A4 },
		noteFrequencies{ InitializeNoteFrequenciesMap() }
	{
		windowCoefficients.resize(PitchAnalysisBuffer::SAMPLES_TO_ANALYZE);

		// Add buffers to queue
		for (PitchAnalysisBuffer& pitchAnalysisBuffer : pitchAnalysisBufferArray) {
			pitchAnalysisBufferQueue.push(&pitchAnalysisBuffer);
		}
	}

	PitchAnalyzer::~PitchAnalyzer()
	{
	}

	winrt::Windows::Foundation::IAsyncAction PitchAnalyzer::Run(SoundAnalyzedCallback soundAnalyzedCallback) noexcept
	{
		// Initialize AudioInput object
		co_await audioInput.Initialize();
		// Attach callback
		this->soundAnalyzedCallback = soundAnalyzedCallback;
		// Get first buffer from queue and attach it to AudioInput class object
		audioInput.AttachBuffer(pitchAnalysisBufferQueue.front());
		// Pass function as callback
		audioInput.BufferFilled(std::bind(&PitchAnalyzer::AudioInput_BufferFilled, this, std::placeholders::_1, std::placeholders::_2));
		// Generate window coefficients
		DSP::WindowGenerator::Generate(DSP::WindowGenerator::WindowType::Hann, windowCoefficients.begin(), windowCoefficients.end());
		// Start recording input
		audioInput.Start();
	}
}