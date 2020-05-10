#pragma once

namespace winrt::Tuner::implementation
{
	constexpr int32_t SAMPLES_TO_ANALYZE{ 1 << 18 };

	struct PitchAnalysisBuffer
	{
		fftwf_plan fftPlan;
		std::array<float, SAMPLES_TO_ANALYZE> audioBuffer;
		std::array<std::complex<float>, SAMPLES_TO_ANALYZE / 2U + 1U> fftResult;

		PitchAnalysisBuffer() {
			fftPlan = fftwf_plan_dft_r2c_1d(
				static_cast<int>(SAMPLES_TO_ANALYZE),
				audioBuffer.data(),
				reinterpret_cast<fftwf_complex*>(fftResult.data()),
				FFTW_MEASURE);
		}

		~PitchAnalysisBuffer() {
			fftwf_destroy_plan(fftPlan);
		}

		void ExecuteFFT() {
			fftwf_execute(fftPlan);
		}
	};
}