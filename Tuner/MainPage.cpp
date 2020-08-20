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

		// Gather dots in array
		dots[0] = Dot0();
		dots[1] = Dot1();
		dots[2] = Dot2();
		dots[3] = Dot3();
		dots[4] = Dot4();
		dots[5] = Dot5();
		dots[6] = Dot6();
		dots[7] = Dot7();
		dots[8] = Dot8();
		dots[9] = Dot9();
		dots[10] = Dot10();
		dots[11] = Dot11();
		dots[12] = Dot12();
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
		co_await SetStateAsync(MainPageState::Loading);

		InitializationStatus initResult{ co_await InitializeFunctionality() };

		if (initResult != InitializationStatus::Success) {
			co_await SetStateAsync(MainPageState::Error);
		}

		co_await SetStateAsync(MainPageState::Tuning);
	}

	std::future<MainPage::InitializationStatus> MainPage::InitializeFunctionality()
	{
		AudioInputInitializationStatus initStatus{ co_await audioInput.InitializeAsync() };

		if (initStatus != AudioInputInitializationStatus::Success) {
			co_return InitializationStatus::Failure;
		}

		pitchAnalyzer.SetSamplingFrequency(static_cast<float>(audioInput.GetSampleRate()));
		pitchAnalyzer.SoundAnalyzed([this](const std::string& note, float frequency, float cents) { 
			SoundAnalyzed_Callback(note, frequency, cents); 
		});

		co_await pitchAnalyzer.InitializeAsync();

		// Attach callback function
		audioInput.BufferFilled([&](const AudioInput& sender, sample_t* first, sample_t* last) {
			// Run harmonic analysis
			pitchAnalyzer.Analyze(first, last);
		});
		audioInput.Start();

		co_return InitializationStatus::Success;
	}

	IAsyncAction MainPage::SoundAnalyzed_Callback(const std::string& note, float frequency, float cents)
	{
		co_await resume_foreground(TuningScreen().Dispatcher());

		// Put the nearest note on the screen
		Note_TextBlock().Text(to_hstring(note));

		// Note in tune
		if (cents <= 2.0f && cents >= -2.0f) {
			ColorForeground(6, 6, Color::Green());
		}
		// Notes above the desired frequency
		else if (cents > 2.0f && cents <= 5.0f) {
			ColorForeground(6, 7, Color::Green());
		}
		else if (cents > 5.0f && cents <= 10.0f) {
			ColorForeground(6, 8, Color::Orange());
		}
		else if (cents > 10.0f && cents <= 15.0f) {
			ColorForeground(6, 9, Color::Orange());
		}
		else if (cents > 15.0f && cents <= 300.0f) {
			ColorForeground(6, 9, Color::Red());
		}
		else if (cents > 300.0f && cents <= 600.0f) {
			ColorForeground(6, 10, Color::Red());
		}
		else if (cents > 600.0f && cents <= 700.0f) {
			ColorForeground(6, 11, Color::Red());
		}
		else if (cents > 700.0f && cents <= 1200.0f) {
			ColorForeground(6, 12, Color::Red());
		}
		// Notes below the desired frequency
		else if (cents < -2.0f && cents >= -5.0f) {
			ColorForeground(5, 6, Color::Green());
		}
		else if (cents < -5.0f && cents >= -10.0f) {
			ColorForeground(4, 6, Color::Orange());
		}
		else if (cents < -10.0f && cents >= -15.0f) {
			ColorForeground(3, 6, Color::Orange());
		}
		else if (cents < -15.0f && cents >= -300.0f) {
			ColorForeground(3, 6, Color::Red());
		}
		else if (cents < -300.0f && cents >= -600.0f) {
			ColorForeground(2, 6, Color::Red()); 
		}
		else if (cents < -600.0f && cents >= -900.0f) {
			ColorForeground(1, 6, Color::Red());
		}
		else if (cents < -900.0f && cents >= -1200.0f) {
			ColorForeground(0, 6, Color::Red());
		}
	}

	IAsyncAction MainPage::SetStateAsync(MainPageState state)
	{
		co_await resume_foreground(Note_TextBlock().Dispatcher());

		switch (state) {
		case MainPageState::Loading:
			LoadingScreen().Visibility(Visibility::Visible);
			TuningScreen().Visibility(Visibility::Collapsed);
			break;
		case MainPageState::Tuning:
			LoadingScreen().Visibility(Visibility::Collapsed);
			TuningScreen().Visibility(Visibility::Visible);
			break;
		default: 
			this->Frame().Navigate(xaml_typename<Tuner::ErrorPage>()); 
			break;
		}
	}

	void MainPage::ColorForeground(int indexMin, int indexMax, const SolidColorBrush& color)
	{
		Note_TextBlock().Foreground(color);
		for (int i = 0; i < dots.size(); i++) {
			if (i >= indexMin && i <= indexMax) {
				dots[i].Fill(color);
			}
			else {
				dots[i].Fill(Color::Gray());
			}
		}
	}
}