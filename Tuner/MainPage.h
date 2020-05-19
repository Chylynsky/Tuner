#pragma once

#include "MainPage.g.h"
#include "PitchAnalyzer.h"

namespace winrt::Tuner::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        AudioInput audioInput;
		PitchAnalyzer pitchAnalyzer;

        MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        winrt::Windows::Foundation::IAsyncAction InitializeFunctionality();
        winrt::Windows::Foundation::IAsyncAction SoundAnalyzed_Callback(const std::string& note, float frequency, float cents);
    };
}

namespace winrt::Tuner::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
