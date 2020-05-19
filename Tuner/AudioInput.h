#pragma once

namespace winrt::Tuner::implementation
{
	MIDL_INTERFACE("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")
		IMemoryBufferByteAccess : ::IUnknown
	{
		virtual HRESULT __stdcall GetBuffer(unsigned char** value, unsigned int* capacity);
	};

	class AudioInput
	{
		using BufferFilledCallback = std::function<void(AudioInput& sender, std::pair<float*, float*> args)>;

		// BufferFilled event handler
		BufferFilledCallback bufferFilledCallback;

		winrt::Windows::Media::Audio::AudioGraph audioGraph;
		winrt::Windows::Media::Audio::AudioGraphSettings audioSettings;
		winrt::Windows::Media::Audio::AudioDeviceInputNode inputDevice;
		winrt::Windows::Media::Audio::AudioFrameOutputNode frameOutputNode;

		std::pair<float*, float*> audioBufferIters;
		// Helper pointers
		float* first;
		float* last;
		float* current;

		void audioGraph_QuantumStarted(winrt::Windows::Media::Audio::AudioGraph const& sender, winrt::Windows::Foundation::IInspectable const args);

	public:

		AudioInput();
		~AudioInput();

		// Get an instance of AudioInput class
		winrt::Windows::Foundation::IAsyncAction Initialize();
		// Start recording audio data
		void Start() const noexcept;
		// Stop recording audio data
		void Stop() const noexcept;

		void AttachBuffer(std::pair<float*, float*> audioBufferIters) noexcept;

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

	inline void AudioInput::AttachBuffer(std::pair<float*, float*> audioBufferIters) noexcept
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