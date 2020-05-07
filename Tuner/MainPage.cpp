﻿#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace std;
using namespace std::placeholders;
using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::Foundation;

namespace winrt::Tuner::implementation
{
	MainPage::MainPage() : pitchAnalyzer{ 440.0f, 20.0f, 8000.0f }
    {
        InitializeComponent();
    }

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

	IAsyncAction MainPage::SoundAnalyzed_Callback(const string& note, float cents, float frequency)
	{
		co_await winrt::resume_foreground(Note_TextBlock().Dispatcher());

		// Get the nearest note on the screen
		Note_TextBlock().Text(to_hstring(note));

		// ... and control its color and circles below, depending on the accuracy of recorded sound
		// Note acuurate
		if (abs(cents) <= 1.0f) {
			Note_TextBlock().Foreground(Media::SolidColorBrush(Windows::UI::Colors::YellowGreen()));

			Lowest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Low().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Middle().Fill(Media::SolidColorBrush(Windows::UI::Colors::YellowGreen()));
			High().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Highest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
		}
		// Note too low
		else if (cents < -1.0f && cents > -6.0f) {
			Note_TextBlock().Foreground(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));

			Lowest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Low().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
			Middle().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			High().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Highest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
		}
		// Note way too low
		else if (cents <= -6.0f) {
			Note_TextBlock().Foreground(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));

			Lowest().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
			Low().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
			Middle().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			High().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Highest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
		}
		// Note too high
		else if (cents > 1.0f && cents < 6.0f) {
			Note_TextBlock().Foreground(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));

			Lowest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Low().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Middle().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			High().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
			Highest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
		}
		// Note way too high
		else if (cents >= 6.0f) {
			Note_TextBlock().Foreground(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));

			Lowest().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Low().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			Middle().Fill(Media::SolidColorBrush(Windows::UI::Colors::Transparent()));
			High().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
			Highest().Fill(Media::SolidColorBrush(Windows::UI::Colors::DarkRed()));
		}
	}

	IAsyncAction MainPage::Page_Loaded(IInspectable const& sender, RoutedEventArgs const& e)
	{
		co_await pitchAnalyzer.Run(std::bind(&MainPage::SoundAnalyzed_Callback, this, _1, _2, _3));
	}
}
