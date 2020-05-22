#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace std;
using namespace placeholders;
using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Foundation;
using namespace Windows::System;

namespace winrt::Tuner::implementation
{
	MainPage::MainPage()
    {
        InitializeComponent();
		InitializeFunctionality();
    }

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

	IAsyncAction MainPage::InitializeFunctionality()
	{
		AudioInputInitializationStatus initStatus{ co_await audioInput.InitializeAsync() };

		if (initStatus != AudioInputInitializationStatus::Success) {
			co_return;
		}

		pitchAnalyzer.SoundAnalyzed(bind(&MainPage::SoundAnalyzed_Callback, this, _1, _2, _3));
		pitchAnalyzer.SetSamplingFrequency(static_cast<float>(audioInput.GetSampleRate()));
		co_await pitchAnalyzer.InitializeAsync();
		// Get first buffer from queue and attach it to AudioInput class object
		audioInput.AttachBuffer(pitchAnalyzer.GetNextAudioBufferIters());
		// Attach callback function
		audioInput.BufferFilled(bind(&MainPage::AudioInput_BufferFilled, this, _1, _2));
		audioInput.Start();
	}

	IAsyncAction MainPage::SoundAnalyzed_Callback(const std::string& note, float frequency, float cents)
	{
		co_await resume_foreground(Note_TextBlock().Dispatcher());

		// Put the nearest note on the screen
		Note_TextBlock().Text(to_hstring(note));

		// ... and control its color and circles below, depending on the accuracy of recorded sound
		// Note acuurate
		if (abs(cents) <= 1.0f) {
			Note_TextBlock().Foreground(SolidColorBrush(Colors::YellowGreen()));

			Lowest().Fill(SolidColorBrush(Colors::Transparent()));
			Low().Fill(SolidColorBrush(Colors::Transparent()));
			Middle().Fill(SolidColorBrush(Colors::YellowGreen()));
			High().Fill(SolidColorBrush(Colors::Transparent()));
			Highest().Fill(SolidColorBrush(Colors::Transparent()));
		}
		// Note too low
		else if (cents < -1.0f && cents > -6.0f) {
			Note_TextBlock().Foreground(SolidColorBrush(Colors::DarkRed()));

			Lowest().Fill(SolidColorBrush(Colors::Transparent()));
			Low().Fill(SolidColorBrush(Colors::DarkRed()));
			Middle().Fill(SolidColorBrush(Colors::Transparent()));
			High().Fill(SolidColorBrush(Colors::Transparent()));
			Highest().Fill(SolidColorBrush(Colors::Transparent()));
		}
		// Note way too low
		else if (cents <= -6.0f) {
			Note_TextBlock().Foreground(SolidColorBrush(Colors::DarkRed()));

			Lowest().Fill(SolidColorBrush(Colors::DarkRed()));
			Low().Fill(SolidColorBrush(Colors::DarkRed()));
			Middle().Fill(SolidColorBrush(Colors::Transparent()));
			High().Fill(SolidColorBrush(Colors::Transparent()));
			Highest().Fill(SolidColorBrush(Colors::Transparent()));
		}
		// Note too high
		else if (cents > 1.0f && cents < 6.0f) {
			Note_TextBlock().Foreground(SolidColorBrush(Colors::DarkRed()));

			Lowest().Fill(SolidColorBrush(Colors::Transparent()));
			Low().Fill(SolidColorBrush(Colors::Transparent()));
			Middle().Fill(SolidColorBrush(Colors::Transparent()));
			High().Fill(SolidColorBrush(Colors::DarkRed()));
			Highest().Fill(SolidColorBrush(Colors::Transparent()));
		}
		// Note way too high
		else if (cents >= 6.0f) {
			Note_TextBlock().Foreground(SolidColorBrush(Colors::DarkRed()));

			Lowest().Fill(SolidColorBrush(Colors::Transparent()));
			Low().Fill(SolidColorBrush(Colors::Transparent()));
			Middle().Fill(SolidColorBrush(Colors::Transparent()));
			High().Fill(SolidColorBrush(Colors::DarkRed()));
			Highest().Fill(SolidColorBrush(Colors::DarkRed()));
		}
	}

	void MainPage::AudioInput_BufferFilled(const AudioInput& sender, const std::pair<float*, float*>& args) noexcept
	{
		// Attach new buffer
		audioInput.AttachBuffer(pitchAnalyzer.GetNextAudioBufferIters());
		// Run harmonic analysis
		async(launch::async, bind(&PitchAnalyzer::Analyze, &pitchAnalyzer, args));
	}
}