#pragma once

namespace winrt::Tuner::implementation
{
	MIDL_INTERFACE("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D") IMemoryBufferByteAccess : ::IUnknown
	{
		virtual HRESULT __stdcall GetBuffer(unsigned char** value, unsigned int* capacity) = 0;
	};

	class AudioInput
	{
	public:

		static constexpr size_t s_audioBufferSize{ 131072U };
		static constexpr size_t s_sampleBufferCount{ 4U };

		using sample_t				= float;
		using SampleBuffer			= std::array<sample_t, s_audioBufferSize>;
		using BufferIterator		= SampleBuffer::iterator;
		using SampleBufferQueue		= std::queue<SampleBuffer*>;
		using SampleBufferArray		= std::array<SampleBuffer, s_sampleBufferCount>;
		using BufferFilledCallback	= std::function<void(BufferIterator first, BufferIterator last)>;
		using AsyncCallbackQueue	= std::queue<std::future<void>>;

	private:

		// BufferFilled event handler
		BufferFilledCallback	bufferFilledCallback;
		// Keep std::futures with asynchronously running callbacks in a queue form
		AsyncCallbackQueue		asyncCallbackQueue;

		winrt::Windows::Media::Audio::AudioGraph			audioGraph;
		winrt::Windows::Media::Audio::AudioGraphSettings	audioSettings;
		winrt::Windows::Media::Audio::AudioDeviceInputNode	inputDevice;
		winrt::Windows::Media::Audio::AudioFrameOutputNode	frameOutputNode;

		SampleBufferArray	sampleBufferArray;
		SampleBufferQueue	sampleBufferQueue;
		SampleBuffer*		sampleBufferPtr;

		// Helper iterators
		BufferIterator first;
		BufferIterator last;
		BufferIterator current;

		void audioGraph_QuantumStarted(winrt::Windows::Media::Audio::AudioGraph const& sender, winrt::Windows::Foundation::IInspectable const args);
		void SwapBuffers();
		void RunCallbackAsync();

	public:

		AudioInput();

		// Get an instance of AudioInput class
		winrt::Windows::Foundation::IAsyncOperation<bool> InitializeAsync();
		// Start recording audio data
		void Start() const;
		// Stop recording audio data
		void Stop() const;
		// Attach buffer filled callback
		void BufferFilled(BufferFilledCallback bufferFilledCallback) noexcept;

		// Get current sample rate
		uint32_t GetSampleRate() const noexcept;
		// Get current bit depth
		uint32_t GetBitDepth() const;
	};

	inline void AudioInput::RunCallbackAsync()
	{
		asyncCallbackQueue.pop();
		asyncCallbackQueue.push(std::async(
			std::launch::async,
			[this]() {
				bufferFilledCallback(sampleBufferPtr->begin(), sampleBufferPtr->end());
			})
		);
	}

	inline void AudioInput::Start() const
	{
		audioGraph.Start();
	}

	inline void AudioInput::Stop() const
	{
		audioGraph.Stop();
	}

	inline void AudioInput::BufferFilled(BufferFilledCallback callback) noexcept
	{
		this->bufferFilledCallback = callback;
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