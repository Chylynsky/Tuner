#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace std::placeholders;
using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::Foundation;
using namespace Windows::System;

namespace winrt::Tuner::implementation
{
	MainPage::MainPage()
    {
        InitializeComponent();
    }

	MainPage::~MainPage()
	{
	}

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

	IAsyncAction MainPage::Page_Loaded(IInspectable const& sender, RoutedEventArgs const& e)
	{
		bool initResult{ co_await InitializeFunctionality() };

		if (!initResult) {
			this->Frame().Navigate(xaml_typename<Tuner::ErrorPage>());
		}
	}

	IAsyncOperation<bool> MainPage::InitializeFunctionality()
	{
		AudioInputInitializationStatus initStatus{ co_await audioInput.InitializeAsync() };

		if (initStatus != AudioInputInitializationStatus::Success) {
			co_return false;
		}

		pitchAnalyzer.SoundAnalyzed(bind(&MainPage::SoundAnalyzed_Callback, this, _1, _2, _3));
		pitchAnalyzer.SetSamplingFrequency(static_cast<float>(audioInput.GetSampleRate()));

		co_await pitchAnalyzer.InitializeAsync();

		// Get first buffer from queue and attach it to AudioInput class object
		audioInput.AttachBuffer(pitchAnalyzer.GetNextAudioBufferIters());
		// Attach callback function
		audioInput.BufferFilled(bind(&MainPage::AudioInput_BufferFilled, this, _1, _2));
		audioInput.Start();

		co_return true;
	}

	IAsyncAction MainPage::SoundAnalyzed_Callback(const std::string& note, float frequency, float cents)
	{
		co_await resume_foreground(Note_TextBlock().Dispatcher());

		// Put the nearest note on the screen
		Note_TextBlock().Text(to_hstring(note));
		Dot_Viewbox().Margin({ 0.0, 0.0, cents, 0 });

		if (float tmp = std::abs(cents); tmp <= 2.0f) {
			Dot().Fill(SolidColorBrush(Colors::YellowGreen()));
			Note_TextBlock().Foreground(SolidColorBrush(Colors::YellowGreen()));
		}
		else if (tmp > 2.0f && tmp <= 10.0f) {
			Dot().Fill(SolidColorBrush(Colors::Orange()));
			Note_TextBlock().Foreground(SolidColorBrush(Colors::Orange()));
		}
		else {
			Dot().Fill(SolidColorBrush(Colors::DarkRed()));
			Note_TextBlock().Foreground(SolidColorBrush(Colors::DarkRed()));
		}
	}

	void MainPage::AudioInput_BufferFilled(const AudioInput& sender, const std::pair<float*, float*>& args) noexcept
	{
		// Attach new buffer
		audioInput.AttachBuffer(pitchAnalyzer.GetNextAudioBufferIters());
		// Run harmonic analysis
		pitchAnalyzer.Analyze(args);
	}
}