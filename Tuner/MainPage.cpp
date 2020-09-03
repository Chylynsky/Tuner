#include "pch.h"
#include "MainPage.h"
#include "MainPage.g.cpp"

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::Foundation;
using namespace Windows::System;

namespace winrt::Tuner::implementation
{
	MainPage::MainPage() : m_audioInput{}, m_pitchAnalyzer{ s_minFrequency, s_maxFrequency, s_baseNoteFrequency }
    {
        InitializeComponent();

		// Gather dots in array
		m_dotArray[0] = Dot0();
		m_dotArray[1] = Dot1();
		m_dotArray[2] = Dot2();
		m_dotArray[3] = Dot3();
		m_dotArray[4] = Dot4();
		m_dotArray[5] = Dot5();
		m_dotArray[6] = Dot6();
		m_dotArray[7] = Dot7();
		m_dotArray[8] = Dot8();
		m_dotArray[9] = Dot9();
		m_dotArray[10] = Dot10();
		m_dotArray[11] = Dot11();
		m_dotArray[12] = Dot12();
    }

	MainPage::~MainPage()
	{
	}

    int32_t MainPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

	IAsyncAction MainPage::Page_Loaded(IInspectable const& sender, RoutedEventArgs const& e)
	{
		co_await SetStateAsync(MainPageState::Loading);

		bool initResult = co_await InitializeFunctionality();

		if (!initResult) 
		{
			co_await SetStateAsync(MainPageState::Error);
		}

		co_await SetStateAsync(MainPageState::Tuning);
	}

	void MainPage::Page_Unloaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::UI::Xaml::RoutedEventArgs const& e)
	{

	}

	IAsyncOperation<bool> MainPage::InitializeFunctionality()
	{
		bool initStatus = co_await m_audioInput.InitializeAsync();

		if (!initStatus) 
		{
			co_return false;
		}

		m_pitchAnalyzer.SetSamplingFrequency(m_audioInput.GetSampleRate());

		// Set sound analyzed callback
		m_pitchAnalyzer.SoundAnalyzed([this](const std::string& note, float frequency, float cents) { 
			SoundAnalyzed_Callback(note, frequency, cents); 
		});

		co_await m_pitchAnalyzer.InitializeAsync();

		// Attach BufferFilled callback function
		m_audioInput.BufferFilled([this](auto first, auto last) {
			m_pitchAnalyzer.Analyze(first, last);
		});

		m_audioInput.Start();

		co_return true;
	}

	IAsyncAction MainPage::SoundAnalyzed_Callback(const std::string& note, float frequency, float cents)
	{
		co_await resume_foreground(TuningScreen().Dispatcher());

		// Put the nearest note on the screen
		Note_TextBlock().Text(to_hstring(note));

		// Check if range is correct
		WINRT_ASSERT(std::abs(cents) <= 50.0f);

		// Note in tune
		if (cents <= 2.0f && cents >= -2.0f) {
			ColorForeground(6, 6, Color::Green());
		}

		// Notes above the desiRed() frequency
		else if (cents > 2.0f && cents <= 5.0f) {
			ColorForeground(6, 7, Color::Green());
		}
		else if (cents > 4.0f && cents <= 10.0f) {
			ColorForeground(6, 8, Color::Orange());
		}
		else if (cents > 10.0f && cents <= 15.0f) {
			ColorForeground(6, 9, Color::Orange());
		}
		else if (cents > 15.0f && cents <= 20.0f) {
			ColorForeground(6, 9, Color::Orange());
		}
		else if (cents > 20.0f && cents <= 30.0f) {
			ColorForeground(6, 10, Color::Red());
		}
		else if (cents > 30.0f && cents <= 40.0f) {
			ColorForeground(6, 11, Color::Red());
		}
		else if (cents > 40.0f) {
			ColorForeground(6, 12, Color::Red());
		}

		// Notes below the desiRed() frequency
		else if (cents < -2.0f && cents >= -5.0f) {
			ColorForeground(5, 6, Color::Green());
		}
		else if (cents < -5.0f && cents >= -10.0f) {
			ColorForeground(4, 6, Color::Orange());
		}
		else if (cents < -10.0f && cents >= -15.0f) {
			ColorForeground(3, 6, Color::Orange());
		}
		else if (cents < -15.0f && cents >= -20.0f) {
			ColorForeground(3, 6, Color::Orange());
		}
		else if (cents < -20.0f && cents >= -30.0f) {
			ColorForeground(2, 6, Color::Red()); 
		}
		else if (cents < -30.0f && cents >= -40.0f) {
			ColorForeground(1, 6, Color::Red());
		}
		else if (cents < -40.0f) {
			ColorForeground(0, 6, Color::Red());
		}
	}

	IAsyncAction MainPage::SetStateAsync(MainPageState state)
	{
		co_await resume_foreground(Note_TextBlock().Dispatcher());

		switch (state) {
		case MainPageState::Loading:
			LoadingScreen().Visibility(Visibility::Visible);
			TuningScreen().Visibility(Visibility::Collapsed);
			break;
		case MainPageState::Tuning:
			LoadingScreen().Visibility(Visibility::Collapsed);
			TuningScreen().Visibility(Visibility::Visible);
			break;
		default: 
			this->Frame().Navigate(xaml_typename<Tuner::ErrorPage>()); 
			break;
		}
	}

	void MainPage::ColorForeground(int indexMin, int indexMax, const SolidColorBrush& color)
	{
		Note_TextBlock().Foreground(color);
		for (int i = 0; i < m_dotArray.size(); i++) {
			if (i >= indexMin && i <= indexMax) {
				m_dotArray[i].Fill(color);
			}
			else {
				m_dotArray[i].Fill(Color::Gray());
			}
		}
	}
}