#pragma once
#include "TypeAliases.h"

namespace winrt::Tuner::implementation
{
	MIDL_INTERFACE("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D") IMemoryBufferByteAccess : ::IUnknown
	{
		virtual HRESULT __stdcall GetBuffer(unsigned char** value, unsigned int* capacity);
	};

	enum class AudioInputInitializationStatus
	{
		Failure,
		Success
	};

	class AudioInput
	{
		using BufferFilledCallback = std::function<void(const AudioInput& sender, const AudioBufferPtrPair& args)>;

		// BufferFilled event handler
		BufferFilledCallback bufferFilledCallback;

		winrt::Windows::Media::Audio::AudioGraph audioGraph;
		winrt::Windows::Media::Audio::AudioGraphSettings audioSettings;
		winrt::Windows::Media::Audio::AudioDeviceInputNode inputDevice;
		winrt::Windows::Media::Audio::AudioFrameOutputNode frameOutputNode;

		AudioBufferPtrPair audioBufferIters;
		// Helper pointers
		sample_t* first;
		sample_t* last;
		sample_t* current;

		void audioGraph_QuantumStarted(winrt::Windows::Media::Audio::AudioGraph const& sender, winrt::Windows::Foundation::IInspectable const args);

	public:

		AudioInput();

		// Get an instance of AudioInput class
		std::future<AudioInputInitializationStatus> InitializeAsync();
		// Start recording audio data
		void Start() const noexcept;
		// Stop recording audio data
		void Stop() const noexcept;

		void AttachBuffer(const AudioBufferPtrPair& audioBufferIters) noexcept;

		void BufferFilled(BufferFilledCallback bufferFilledCallback) noexcept;

		// Get the number of recorded samples
		uint32_t RecordedDataSize() const noexcept;
		// Get current sample rate
		uint32_t GetSampleRate() const noexcept;
		// Get current bit depth
		uint32_t GetBitDepth() const;
	};

	inline void AudioInput::Start() const noexcept
	{
		audioGraph.Start();
	}

	inline void AudioInput::Stop() const noexcept
	{
		audioGraph.Stop();
	}

	inline void AudioInput::AttachBuffer(const AudioBufferPtrPair& audioBufferIters) noexcept
	{
		this->audioBufferIters = audioBufferIters;
		first = current = audioBufferIters.first;
		last = audioBufferIters.second;
	}

	inline void AudioInput::BufferFilled(BufferFilledCallback bufferFilledCallback) noexcept
	{
		this->bufferFilledCallback = bufferFilledCallback;
	}

	// Get the number of recorded samples
	inline uint32_t AudioInput::RecordedDataSize() const noexcept
	{
		return static_cast<uint32_t>(current - first);
	}

	// Get current sample rate
	inline uint32_t AudioInput::GetSampleRate() const noexcept
	{
		return inputDevice.EncodingProperties().SampleRate();
	}

	// Get current bit depth
	inline uint32_t AudioInput::GetBitDepth() const
	{
		return inputDevice.EncodingProperties().BitsPerSample();
	}
}