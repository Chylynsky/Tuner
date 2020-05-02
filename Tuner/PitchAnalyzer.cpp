#include "pch.h"
#include "PitchAnalyzer.h"

void PitchAnalyzer::GetNote(float frequency)
{
	// Get the nearest note above or equal
	auto high = noteFrequencies.lower_bound(frequency);
	// Get the nearest note below
	auto low = std::prev(high);

	float highDiff = (*high).first - frequency;
	float lowDiff = frequency - (*low).first;

	if (highDiff < lowDiff)
	{
		cents = 1200.0f * std::log2((*high).first / frequency);
		note = (*high).second;
	}
	else
	{
		cents = 1200.0f * std::log2((*low).first / frequency);
		note = (*low).second;
	}
}

void PitchAnalyzer::Analyze(void* instance, void (*callback)(void*, std::string&, float, float))
{
	dev.Initialize();

	while (!quit.load())
	{
		if (dev.RecordedDataSize() >= samplesToAnalyze)
		{
			auto lock{ dev.Lock() };
			fftwf_execute_dft_r2c(fftPlan, dev.GetRawData(), reinterpret_cast<fftwf_complex*>(fftResult.data()));
			GetFirstHarmonic(fftResult.begin(), fftResult.end(), dev.GetSampleRate());

			if (firstHarmonic >= minFrequency && firstHarmonic <= maxFrequency)
			{
				GetNote(firstHarmonic);
				callback(instance, note, cents, firstHarmonic);
			}
			// Clear the audio input device's buffer and start recording again
			dev.ClearData();
		}
		// Sleep
		//std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}

std::map<float, std::string> PitchAnalyzer::InitializeNoteFrequenciesMap()
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
	while (currentFrequency >= minFrequency)
	{
		currentFrequency = baseNoteFrequency * std::pow(a, halfSteps);
		result[currentFrequency] = *octaveIter + std::to_string(currentOctave);
		halfSteps--;

		if (octaveIter == octave.begin())
		{
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
	while (currentFrequency <= maxFrequency)
	{
		if (octaveIter == octave.end())
		{
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

// Sound analysis is performed on its own thread
void PitchAnalyzer::Run(void* instance, void (*callback)(void*, std::string&, float, float))
{
	analysis = std::thread(&PitchAnalyzer::Analyze, this, instance, callback);
}

PitchAnalyzer::PitchAnalyzer(float baseNoteFrequency, float minFrequency, float maxFrequency, size_t samplesToAnalyze) :
	samplesToAnalyze{ samplesToAnalyze }, minFrequency{ minFrequency }, maxFrequency{ maxFrequency }, baseNoteFrequency{ baseNoteFrequency },
	noteFrequencies{ InitializeNoteFrequenciesMap() }, firstHarmonic{ 0.0f }, cents{ 0.0f }, note{ "A4" }
{
	quit.store(false);
	fftResult.resize(samplesToAnalyze / 2ULL + 1ULL);
	fftPlan = fftwf_plan_dft_r2c_1d(samplesToAnalyze, dev.GetRawData(), reinterpret_cast<fftwf_complex*>(fftResult.data()), FFTW_ESTIMATE);
}

PitchAnalyzer::~PitchAnalyzer()
{
	if (!quit.load())
		quit.store(true);

	// Wait for the thread to finish
	if (analysis.joinable())
		analysis.join();

	fftwf_destroy_plan(fftPlan);
}