#pragma once

#include "MainPage.g.h"
#include "PitchAnalyzer.h"

namespace winrt::Tuner::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
		PitchAnalyzer pitchAnalyzer;

        MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);
        winrt::Windows::Foundation::IAsyncAction SoundAnalyzed_Callback(const std::string& note, float cents, float frequency);
        winrt::Windows::Foundation::IAsyncAction Page_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void Page_Unloaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::Tuner::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
