#include "pch.h"
#include "ErrorPage.h"
#if __has_include("ErrorPage.g.cpp")
#include "ErrorPage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;
using namespace Windows::Foundation;
using namespace Windows::System;

namespace winrt::Tuner::implementation
{
    ErrorPage::ErrorPage()
    {
        InitializeComponent();
    }

    int32_t ErrorPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void ErrorPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    IAsyncAction ErrorPage::SettingsButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        bool result = co_await Launcher::LaunchUriAsync(Uri(L"ms-settings:privacy-microphone"));
        WINRT_ASSERT(result);
    }


    void ErrorPage::RetryButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        this->Frame().Navigate(xaml_typename<Tuner::MainPage>());
    }
}