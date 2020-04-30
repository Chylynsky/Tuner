#pragma once

#include "MainPage.g.h"
#include "PitchAnalyzer.h"

namespace winrt::Tuner::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
		PitchAnalyzer p;

        MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

		static void SoundAnalyzed_Callback(void* instance, std::string& note, float inaccuracy, float frequency);
		Windows::Foundation::IAsyncAction UpdateTunerScreenAsync(std::string& note, float inaccuracy, float frequency);
    };
}

namespace winrt::Tuner::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
