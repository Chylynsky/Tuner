#pragma once
#include "MainPage.g.h"
#include "PitchAnalyzer.h"
#include "ErrorPage.h"

namespace winrt::Tuner::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        AudioInput audioInput;
		PitchAnalyzer pitchAnalyzer;

        MainPage();
        ~MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        winrt::Windows::Foundation::IAsyncAction Page_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncOperation<bool> InitializeFunctionality();
        winrt::Windows::Foundation::IAsyncAction SoundAnalyzed_Callback(const std::string& note, float frequency, float cents);
        void AudioInput_BufferFilled(const AudioInput& sender, const std::pair<float*, float*>& args) noexcept;
    };
}

namespace winrt::Tuner::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
