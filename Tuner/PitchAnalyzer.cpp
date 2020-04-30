#include "pch.h"
#include "PitchAnalyzer.h"

const long PitchAnalyzer::samplesToAnalyze{ 32768 };
const float PitchAnalyzer::minFrequency{ 15 };
const float PitchAnalyzer::maxFrequency{ 8000 };
//const float PitchAnalyzer::minAmplitude{ 40 };
const std::vector<std::string> PitchAnalyzer::octave{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
const float PitchAnalyzer::a{ std::pow(2.0f, 1.0f / 12.0f) };

void PitchAnalyzer::GetNote(float frequency)
{
	// Difference in half tones between currently analysed frequency and A4 note
	float halfTones = std::log10(frequency / A4) / std::log10(a);
	// Split into integer and floating point parts
	long integerPart = static_cast<long>(std::round(halfTones));
	// Save the value
	inaccuracy = halfTones - integerPart;
	// Set iterator at the position of A in the octave
	auto findBaseNote = std::find(octave.begin(), octave.end(), "A");

	// Starting point, A4 note is in fourth octave
	int currentOctave = 4;

	if (integerPart >= 0)
	{
		// Circle around a vector until the valid note is found
		while (integerPart > 0)
		{
			integerPart--;
			findBaseNote++;
			if (findBaseNote == octave.end())
			{
				// Go one octave up
				currentOctave++;
				findBaseNote = octave.begin();
			}
		}
	}
	else
	{
		// Circle around a vector until the valid note is found
		while (integerPart != 0)
		{
			if (findBaseNote == octave.begin())
			{
				// Go one octave down
				currentOctave--;
				findBaseNote = octave.end();
			}

			integerPart++;
			findBaseNote--;
		}
	}
	note = *findBaseNote + std::to_string(currentOctave);
}

void PitchAnalyzer::Analyze(void* instance, void (*callback)(void*, std::string&, float, float))
{
	dev.Initialize();

	while (!quit)
	{
		if (dev.RecordedDataSize() >= samplesToAnalyze)
		{
			fftwf_execute_dft_r2c(fftPlan, dev.GetRawData(), reinterpret_cast<fftwf_complex*>(fftResult.data()));
			GetFirstHarmonic(fftResult.begin(), fftResult.end(), dev.GetSampleRate());

			if (firstHarmonic >= minFrequency && firstHarmonic <= maxFrequency)
			{
				GetNote(firstHarmonic);
				callback(instance, note, inaccuracy, firstHarmonic);
			}
			// Clear the audio input device's buffer and start recording again
			dev.ClearData();
		}
		// Sleep
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}

// Sound analysis is performed on its own thread
void PitchAnalyzer::Run(void* instance, void (*callback)(void*, std::string&, float, float))
{
	analysis = std::thread(&PitchAnalyzer::Analyze, this, instance, callback);
}

PitchAnalyzer::PitchAnalyzer() : A4{ 440 }, firstHarmonic { 0.0f }, maxAmplitude{ 0.0f }, inaccuracy{ 0.0f },
fftResult{ std::vector<std::complex<float>>(samplesToAnalyze / 2 + 1) }, note{ "X" }, quit{ false }
{
	fftPlan = fftwf_plan_dft_r2c_1d(samplesToAnalyze, dev.GetRawData(), reinterpret_cast<fftwf_complex*>(fftResult.data()), FFTW_ESTIMATE);
}

PitchAnalyzer::~PitchAnalyzer()
{
	if (!quit)
		quit = true;

	// Wait for the thread to finish
	if (analysis.joinable())
		analysis.join();

	fftwf_destroy_plan(fftPlan);
}