#pragma once
#include "MainPage.g.h"
#include "PitchAnalyzer.h"
#include "ErrorPage.h"
#include "TypeAliases.h"

namespace winrt::Tuner::implementation
{
    struct MainPage : MainPageT<MainPage>
    {
        using DotArray = std::array<winrt::Windows::UI::Xaml::Shapes::Ellipse, 13>;

        enum class InitializationStatus {
            Failure,
            Success
        };

        enum class MainPageState {
            Loading,
            Tuning,
            Error
        };

        struct Color
        {
            static winrt::Windows::UI::Xaml::Media::SolidColorBrush Gray() noexcept;
            static winrt::Windows::UI::Xaml::Media::SolidColorBrush Red() noexcept;
            static winrt::Windows::UI::Xaml::Media::SolidColorBrush Orange() noexcept;
            static winrt::Windows::UI::Xaml::Media::SolidColorBrush Green() noexcept;
        };

        AudioInput audioInput;
		PitchAnalyzer pitchAnalyzer;
        DotArray dots;

        MainPage();
        ~MainPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        winrt::Windows::Foundation::IAsyncAction Page_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        std::future<InitializationStatus> InitializeFunctionality();

        /*
        *	Function serves as a callback to the PitchAnalyzer objects' SoundAnalyzed event.
        */
        winrt::Windows::Foundation::IAsyncAction SoundAnalyzed_Callback(const std::string& note, float frequency, float cents);

        /*
        *	Set currently visible page depending on the current application state.
        */
        winrt::Windows::Foundation::IAsyncAction SetStateAsync(MainPageState state);

        /*
        *	Color the text and dots in a given range. Dots from indexMin to indexMax will be colored
        *	with the specified color.
        */
        void ColorForeground(int indexMin, int indexMax, const winrt::Windows::UI::Xaml::Media::SolidColorBrush& color);
    };

    inline winrt::Windows::UI::Xaml::Media::SolidColorBrush MainPage::Color::Gray() noexcept
    {
        return winrt::Windows::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::DarkGray());
    }

    inline winrt::Windows::UI::Xaml::Media::SolidColorBrush MainPage::Color::Red() noexcept
    {
        return winrt::Windows::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::DarkRed());
    }

    inline winrt::Windows::UI::Xaml::Media::SolidColorBrush MainPage::Color::Orange() noexcept
    {
        return winrt::Windows::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Orange());
    }

    inline winrt::Windows::UI::Xaml::Media::SolidColorBrush MainPage::Color::Green() noexcept
    {
        return winrt::Windows::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::YellowGreen());
    }
}

namespace winrt::Tuner::factory_implementation
{
    struct MainPage : MainPageT<MainPage, implementation::MainPage>
    {
    };
}
