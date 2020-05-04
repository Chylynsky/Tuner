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
    };
}

namespace winrt::Tuner::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
