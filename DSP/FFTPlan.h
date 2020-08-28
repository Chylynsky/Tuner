#pragma once
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <hstring.h>
#include <cstdint>
#include <string>
#include <utility>
#include "fftw3.h"
#include "DSPTypeTraits.h"

namespace DSP
{
	template<typename _Ty>
	class FFTManager
	{
		static uint32_t s_refCount;
	public:
		FFTManager() { s_refCount++; }
		~FFTManager() 
		{
			s_refCount--;

			if (!s_refCount)
			{
				if constexpr (Is_float<_Ty>)
				{
					fftwf_cleanup();
				}
				else if constexpr (Is_double<_Ty>)
				{
					fftw_cleanup();
				}
				else if constexpr (Is_long_double<_Ty>)
				{
					fftwl_cleanup();
				}
			}
		}
	};

	template<typename _Ty>
	uint32_t FFTManager<_Ty>::s_refCount = 0;

	enum class flags
	{
		measure = FFTW_MEASURE,
		wisdom	= FFTW_WISDOM_ONLY
	};

	template<typename _Ty>
	class FFTPlan : FFTManager<_Ty>
	{
		static_assert(Is_floating_point<_Ty>, "Value type must be floating point.");

		fftw_plan_type<_Ty> m_fftPlan;

		void DestroyPlan() noexcept
		{
			if constexpr (Is_float<_Ty>)
			{
				fftwf_destroy_plan(m_fftPlan);
			}
			else if constexpr (Is_double<_Ty>)
			{
				fftw_destroy_plan(m_fftPlan);
			}
			else if constexpr (Is_long_double<_Ty>)
			{
				fftwl_destroy_plan(m_fftPlan);
			}

			m_fftPlan = nullptr;
		}

	public:

		static winrt::Windows::Foundation::IAsyncOperation<bool> LoadFFTPlan(winrt::hstring fileName) noexcept
		{
			using namespace winrt::Windows::Storage;

			StorageFolder storageFolder = ApplicationData::Current().LocalFolder();
			IStorageItem storageItem = co_await storageFolder.TryGetItemAsync(fileName);

			if (!storageItem)
			{
				co_return false;
			}

			StorageFile file = storageItem.as<StorageFile>();
			std::string fftPlanBuffer = winrt::to_string(co_await FileIO::ReadTextAsync(file));

			if constexpr (Is_float<_Ty>)
			{
				fftwf_import_wisdom_from_string(fftPlanBuffer.c_str());
			}
			else if constexpr (Is_double<_Ty>)
			{
				fftw_import_wisdom_from_string(fftPlanBuffer.c_str());
			}
			else if constexpr (Is_long_double<_Ty>)
			{
				fftwl_import_wisdom_from_string(fftPlanBuffer.c_str());
			}

			co_return true;
		}

		winrt::Windows::Foundation::IAsyncAction SaveFFTPlan(winrt::hstring fileName) const noexcept
		{
			using namespace winrt::Windows::Storage;

			char* fftPlanBufferRaw = nullptr;

			if constexpr (Is_float<_Ty>)
			{
				fftPlanBufferRaw = fftwf_export_wisdom_to_string();
			}
			else if constexpr (Is_double<_Ty>)
			{
				fftPlanBufferRaw = fftw_export_wisdom_to_string();
			}
			else if constexpr (Is_long_double<_Ty>)
			{
				fftPlanBufferRaw = fftwl_export_wisdom_to_string();
			}

			winrt::hstring fftPlanBuffer = winrt::to_hstring(fftPlanBufferRaw);

			StorageFolder storageFolder = ApplicationData::Current().LocalFolder();
			StorageFile file = co_await storageFolder.CreateFileAsync(fileName, CreationCollisionOption::ReplaceExisting);
			co_await FileIO::WriteTextAsync(file, fftPlanBuffer);
		}

		FFTPlan() : m_fftPlan{ nullptr } 
		{
		};

		FFTPlan(FFTPlan&& other) : m_fftPlan{ nullptr }
		{
			*this = std::move(other);
		}

		template<typename _InIt1, typename _InIt2>
		FFTPlan(_InIt1 _First, _InIt1 _Last, _InIt2 _Dest, flags _flags = flags::measure) : m_fftPlan{ nullptr }
		{
			static_assert(Is_value_type_floating_point<_InIt1>, "Value type must be floating point.");
			static_assert(Is_same<Iterator_value_type<_InIt1>, _Ty>, "Floating point types does not match.");
			static_assert(Is_value_type_complex<_Ty, _InIt2>, "Value type in destination iterator must be of std::complex<_Ty> type.");

			using diff_t = Iterator_difference_type<_InIt1>;

			const diff_t fftSize = std::distance(_First, _Last) / static_cast<diff_t>(2) + static_cast<diff_t>(1);

			if constexpr (Is_float<_Ty>)
			{
				m_fftPlan = fftwf_plan_dft_r2c_1d(fftSize, &(*_First), reinterpret_cast<fftwf_complex*>(&(*_Dest)), static_cast<int>(_flags));
			}
			else if constexpr (Is_double<_Ty>)
			{
				m_fftPlan = fftw_plan_dft_r2c_1d(fftSize, &(*_First), reinterpret_cast<fftw_complex*>(&(*_Dest)), static_cast<int>(_flags));
			}
			else if constexpr (Is_long_double<_Ty>)
			{
				m_fftPlan = fftwl_plan_dft_r2c_1d(fftSize, &(*_First), reinterpret_cast<fftwl_complex*>(&(*_Dest)), static_cast<int>(_flags));
			}
		}

		~FFTPlan() noexcept
		{
			if (m_fftPlan)
			{
				DestroyPlan();
			}
		}

		FFTPlan& operator=(FFTPlan&& other)
		{
			if (this != &other)
			{
				if (m_fftPlan)
				{
					DestroyPlan();
				}
				m_fftPlan = other.m_fftPlan;
				other.m_fftPlan = nullptr;
			}

			return *this;
		}

		operator bool() const noexcept
		{
			return m_fftPlan;
		}

		void Execute() const
		{
#ifdef _DEBUG
			if (!m_fftPlan)
			{
				throw std::runtime_error("FFT plan not created.");
			}
#endif

			if constexpr (Is_float<_Ty>)
			{
				fftwf_execute(m_fftPlan);
			}
			else if constexpr (Is_double<_Ty>)
			{
				fftw_execute(m_fftPlan);
			}
			else if constexpr (Is_long_double<_Ty>)
			{
				fftwl_execute(m_fftPlan);
			}
		}

		template<typename _InIt1, typename _InIt2>
		void Execute(_InIt1 _First, _InIt1 _Last, _InIt2 _Dest) const
		{
			static_assert(Is_value_type_floating_point<_InIt1>, "Value type must be floating point.");
			static_assert(Is_same<Iterator_value_type<_InIt1>, _Ty>, "Floating point types does not match.");
			static_assert(Is_value_type_complex<_Ty, _InIt2>, "Value type in destination iterator must be of std::complex<_Ty> type.");

#ifdef _DEBUG
			if (!m_fftPlan)
			{
				throw std::runtime_error("FFT plan not created.");
			}
#endif

			if constexpr (Is_float<_Ty>)
			{
				fftwf_execute_dft_r2c(m_fftPlan, &(*_First), reinterpret_cast<fftwf_complex*>(&(*_Dest)));
			}
			else if constexpr (Is_double<_Ty>)
			{
				fftw_execute_dft_r2c(m_fftPlan, &(*_First), reinterpret_cast<fftw_complex*>(&(*_Dest)));
			}
			else if constexpr (Is_long_double<_Ty>)
			{
				fftwl_execute_dft_r2c(m_fftPlan, &(*_First), reinterpret_cast<fftwl_complex*>(&(*_Dest)));
			}
		}

		FFTPlan(const FFTPlan&) = delete;
		FFTPlan& operator=(const FFTPlan&) = delete;
	};
}