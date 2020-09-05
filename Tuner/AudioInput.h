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
		using CallbackFuture		= std::future<void>;
		using AsyncCallbackQueue	= std::queue<CallbackFuture>;

	private:

		// BufferFilled event handler
		BufferFilledCallback	m_bufferFilledCallback;
		// Keep std::futures with asynchronously running callbacks in a queue form
		AsyncCallbackQueue		m_asyncCallbackQueue;

		winrt::Windows::Media::Audio::AudioGraph			m_audioGraph;
		winrt::Windows::Media::Audio::AudioGraphSettings	m_audioSettings;
		winrt::Windows::Media::Audio::AudioDeviceInputNode	m_inputDevice;
		winrt::Windows::Media::Audio::AudioFrameOutputNode	m_frameOutputNode;

		SampleBufferArray	m_sampleBufferArray;
		SampleBufferQueue	m_sampleBufferQueue;
		SampleBuffer*		m_pSampleBuffer;

		// Helper iterators
		BufferIterator m_first;
		BufferIterator m_last;
		BufferIterator m_current;

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
		m_asyncCallbackQueue.pop();
		m_asyncCallbackQueue.push(std::async(
			std::launch::async,
			[this]() {
				m_bufferFilledCallback(m_pSampleBuffer->begin(), m_pSampleBuffer->end());
			})
		);
	}

	inline void AudioInput::Start() const
	{
		m_audioGraph.Start();
	}

	inline void AudioInput::Stop() const
	{
		m_audioGraph.Stop();
	}

	inline void AudioInput::BufferFilled(BufferFilledCallback callback) noexcept
	{
		this->m_bufferFilledCallback = callback;
	}

	// Get current sample rate
	inline uint32_t AudioInput::GetSampleRate() const noexcept
	{
		return m_inputDevice.EncodingProperties().SampleRate();
	}

	// Get current bit depth
	inline uint32_t AudioInput::GetBitDepth() const
	{
		return m_inputDevice.EncodingProperties().BitsPerSample();
	}
}