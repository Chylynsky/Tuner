#pragma once
namespace winrt::Tuner::implementation
{
	constexpr uint32_t AUDIO_BUFFER_SIZE{ 1 << 16 };

	using sample_t = float;
	using complex_t = std::complex<sample_t>;

	template<uint32_t bufferSize> 
	using SampleBuffer = std::array<sample_t, bufferSize>;

	template<uint32_t bufferSize>
	using SampleBufferQueue = std::queue<SampleBuffer<bufferSize>*>;

	template<uint32_t bufferSize, uint32_t arraySize>
	using SampleBufferArray = std::array<SampleBuffer<bufferSize>, arraySize>;
}