#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace std;
using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::Foundation;

namespace winrt::Tuner::implementation
{
    MainPage::MainPage()
    {
        InitializeComponent();
		p.Run(this, SoundAnalyzed_Callback);
    }

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

	void MainPage::SoundAnalyzed_Callback(void* instance, string& note, float inaccuracy, float frequency)
	{
		// Get information about the callee, proceed with data to thread-safe function
		reinterpret_cast<MainPage*>(instance)->UpdateTunerScreenAsync(note, inaccuracy, frequency);
	}

	IAsyncAction MainPage::UpdateTunerScreenAsync(string& note, float cents, float frequency)
	{
		co_await winrt::resume_foreground(Note_TextBlock().Dispatcher());

		// Get the nearest note on the screen
		Note_TextBlock().Text(to_hstring(note));

		// ... and control its color and circles below, depending on the accuracy of recorded sound
		// Note acuurate
		if (abs(cents) <= 1.0f)
		{
			Note_TextBlock().Foreground(Media::SolidColorBrush(Windows::UI::Colors::YellowGreen()));

			Lowest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Low().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Middle().Fill(Media::SolidColorBrush(Windows::UI::Colors::YellowGreen()));
			High().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Highest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
		}
		// Note too low
		else if (cents < -1.0f && cents > -6.0f)
		{
			Note_TextBlock().Foreground(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));

			Lowest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Low().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
			Middle().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			High().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Highest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
		}
		// Note way too low
		else if (cents <= -6.0f)
		{
			Note_TextBlock().Foreground(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));

			Lowest().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
			Low().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
			Middle().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			High().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Highest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
		}
		// Note too high
		else if (cents > 1.0f && cents < 6.0f)
		{
			Note_TextBlock().Foreground(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));

			Lowest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Low().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Middle().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			High().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
			Highest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
		}
		// Note way too high
		else if (cents >= 6.0f)
		{
			Note_TextBlock().Foreground(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));

			Lowest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Low().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Middle().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			High().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
			Highest().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
		}
	}
}
