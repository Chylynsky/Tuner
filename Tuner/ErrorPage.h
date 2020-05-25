#pragma once
#include "ErrorPage.g.h"

namespace winrt::Tuner::implementation
{
    struct ErrorPage : ErrorPageT<ErrorPage>
    {
        ErrorPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);
        winrt::Windows::Foundation::IAsyncAction SettingsButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
        void RetryButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::Tuner::factory_implementation
{
    struct ErrorPage : ErrorPageT<ErrorPage, implementation::ErrorPage>
    {
    };
}
