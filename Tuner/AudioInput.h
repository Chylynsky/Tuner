#pragma once

MIDL_INTERFACE("5B0D3235-4DBA-4D44-865E-8F1D0E4FD04D")
IMemoryBufferByteAccess : ::IUnknown
{
	virtual HRESULT __stdcall GetBuffer(unsigned char** value, unsigned int* capacity);
};

class AudioInput
{
	using sample = float;

	winrt::Windows::Media::Audio::AudioGraph audioGraph;
	winrt::Windows::Media::Audio::AudioGraphSettings audioSettings;
	winrt::Windows::Media::Audio::AudioDeviceInputNode inputDevice;
	winrt::Windows::Media::Audio::AudioFrameOutputNode frameOutputNode;
	std::vector<sample> audioBlock;
	std::mutex audioInputMutex;

	void audioGraph_QuantumStarted(winrt::Windows::Media::Audio::AudioGraph const& sender, winrt::Windows::Foundation::IInspectable const args);

public:

	AudioInput();
	~AudioInput();

	// Get an instance of AudioInput class
	winrt::Windows::Foundation::IAsyncAction Initialize();
	// Get the number of recorded samples
	size_t RecordedDataSize();
	// Get current sample rate
	unsigned int GetSampleRate();
	// Get current bit depth
	unsigned int GetBitDepth();
	// Clear the buffer containing recorded samples
	void ClearData();
	// Get raw pointer to recorded data
	sample* GetRawData();
	// Get iterator to the first sample
	std::vector<sample>::iterator FirstFrameIterator();
	// Lock resource
	std::lock_guard<std::mutex> Lock();
};