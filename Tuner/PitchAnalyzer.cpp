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

	void PitchAnalyzer::Analyze(std::function<void(const std::string&, float, float)> callback) noexcept
	{
		while (!quitAnalysis.load()) {
			if (audioInput.RecordedDataSize() >= SAMPLES_TO_ANALYZE) {
				audioInput.Stop();
				auto lock = audioInput.LockAudioInputDevice();
				auto* sample = audioInput.GetRawPtr();

				// Apply window function in 4 threads
				DSP::WindowGenerator::ApplyWindow(sample, sample + SAMPLES_TO_ANALYZE, windowCoefficients.begin(), 4);
				fftwf_execute_dft_r2c(fftPlan, sample, reinterpret_cast<fftwf_complex*>(fftResult.data()));
				float firstHarmonic{ GetFirstHarmonic(fftResult.begin(), fftResult.end(), audioInput.GetSampleRate()) };

				if (firstHarmonic >= minFrequency && firstHarmonic <= maxFrequency) {
					PitchAnalysisResult measurement{ GetNote(firstHarmonic) };
					callback(measurement.note, measurement.cents, firstHarmonic);
				}
				audioInput.ClearData();
			}
			// Sleep for the time needed to fill the buffer
			audioInput.Start();
			std::this_thread::sleep_for(std::chrono::milliseconds(1000 * SAMPLES_TO_ANALYZE / audioInput.GetSampleRate()));
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

	PitchAnalyzer::PitchAnalyzer(float A4, float minFrequency, float maxFrequency) :
		minFrequency{ minFrequency },
		maxFrequency{ maxFrequency },
		baseNoteFrequency{ A4 },
		noteFrequencies{ InitializeNoteFrequenciesMap() },
		quitAnalysis{ false }
	{

	}

	PitchAnalyzer::~PitchAnalyzer()
	{
		// Terminate the infinite loop
		if (!quitAnalysis.load()) {
			quitAnalysis.store(true);
		}

		// Join the thread
		if (pitchAnalysisThread.joinable()) {
			pitchAnalysisThread.join();
		}

		fftwf_destroy_plan(fftPlan);
		fftwf_cleanup();
	}
}