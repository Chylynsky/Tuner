#pragma once

namespace winrt::Tuner::implementation
{
	struct PitchAnalysisBuffer
	{
		static constexpr int32_t SAMPLES_TO_ANALYZE{ 1 << 14 };

		std::mutex mtx;

		fftwf_plan fftPlan;
		std::vector<float> audioBuffer;
		std::vector<std::complex<float>> fftResult;

		PitchAnalysisBuffer() {
			audioBuffer.resize(SAMPLES_TO_ANALYZE);
			fftResult.resize(SAMPLES_TO_ANALYZE / 2U + 1U);
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

		std::lock_guard<std::mutex> LockBuffer() {
			return std::lock_guard<std::mutex>(mtx);
		}
	};
}