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
		using sample_t = float;

		winrt::Windows::Media::Audio::AudioGraph audioGraph;
		winrt::Windows::Media::Audio::AudioGraphSettings audioSettings;
		winrt::Windows::Media::Audio::AudioDeviceInputNode inputDevice;
		winrt::Windows::Media::Audio::AudioFrameOutputNode frameOutputNode;
		std::vector<sample_t> audioBuffer;
		std::mutex audioInputMutex;

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
		// Set the size of audio buffer
		void SetAudioBufferSize(size_t newSize);
		// Get the number of recorded samples
		size_t RecordedDataSize() const noexcept;
		// Get current sample rate
		uint32_t GetSampleRate() const noexcept;
		// Get current bit depth
		uint32_t GetBitDepth() const;
		// Clear the buffer containing recorded samples
		void ClearData() noexcept;
		// Get raw pointer to recorded data
		sample_t* GetRawData() noexcept;
		// Get iterator to the first sample
		std::vector<sample_t>::iterator FirstFrameIterator() noexcept;
		// Lock resource
		std::lock_guard<std::mutex> LockAudioInputDevice() noexcept;
	};

	inline void AudioInput::Start() const noexcept
	{
		audioGraph.Start();
	}

	inline void AudioInput::Stop() const noexcept
	{
		audioGraph.Stop();
	}

	inline void AudioInput::SetAudioBufferSize(size_t newSize)
	{
		audioBuffer.resize(newSize);
	}

	// Get the number of recorded samples
	inline size_t AudioInput::RecordedDataSize() const noexcept
	{
		return audioBuffer.size();
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

	// Clear the buffer containing recorded samples
	inline void AudioInput::ClearData() noexcept
	{
		audioBuffer.clear();
	}

	// Get raw pointer to recorded data
	inline AudioInput::sample_t* AudioInput::GetRawData() noexcept
	{
		return audioBuffer.data();
	}

	// Get iterator to the first sample
	inline std::vector<AudioInput::sample_t>::iterator AudioInput::FirstFrameIterator() noexcept
	{
		return audioBuffer.begin();
	}

	inline std::lock_guard<std::mutex> AudioInput::LockAudioInputDevice() noexcept
	{
		return std::lock_guard<std::mutex>(audioInputMutex);
	}
}