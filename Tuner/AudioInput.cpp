#include "pch.h"
#include "AudioInput.h"

using namespace std;
using namespace winrt;
using namespace winrt::Windows;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Media::Audio;
using namespace winrt::Windows::Media::Capture;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Media::MediaProperties;

// Get an instance of AudioInput class
IAsyncAction AudioInput::Initialize()
{
	audioSettings = AudioGraphSettings(Render::AudioRenderCategory::Media);
	// Start async operation that creates new audio graph
	CreateAudioGraphResult graphCreation{ co_await AudioGraph::CreateAsync(audioSettings) };

	// Check if succesfully created
	if (graphCreation.Status() != AudioGraphCreationStatus::Success)
		throw runtime_error("AudioGraph creation failed.");
	else
	{
		// Get created graph
		audioGraph = graphCreation.Graph();
		// Create output node for recorded data
		frameOutputNode = audioGraph.CreateFrameOutputNode();
		// Attach callback
		audioGraph.QuantumStarted({ this, &AudioInput::audioGraph_QuantumStarted });

		// Start audio input device node creation
		CreateAudioDeviceInputNodeResult nodeCreation{ co_await audioGraph.CreateDeviceInputNodeAsync(MediaCategory::Media) };

		// Check if succesful
		if (nodeCreation.Status() != AudioDeviceNodeCreationStatus::Success)
			throw runtime_error("AudioDeviceInputNode creation failed.");
		else
		{
			inputDevice = nodeCreation.DeviceInputNode();
			// Input from the recording device is routed to frameOutputNode
			inputDevice.AddOutgoingConnection(frameOutputNode);
			audioGraph.Start();
		}
	}
}

// Get the number of recorded samples
size_t AudioInput::RecordedDataSize()
{
	return audioBlock.size();
}

// Get current sample rate
unsigned int AudioInput::GetSampleRate()
{
	return inputDevice.EncodingProperties().SampleRate();
}

// Get current bit depth
unsigned int AudioInput::GetBitDepth()
{
	return inputDevice.EncodingProperties().BitsPerSample();
}

// Clear the buffer containing recorded samples
void AudioInput::ClearData()
{
	audioBlock.clear();
}

// Get raw pointer to recorded data
AudioInput::sample* AudioInput::GetRawData()
{
	return audioBlock.data();
}

// Get iterator to the first sample
std::vector<AudioInput::sample>::iterator AudioInput::FirstFrameIterator()
{
	return audioBlock.begin();
}

std::lock_guard<std::mutex> AudioInput::Lock()
{
	return std::lock_guard<std::mutex>(audioInputMutex);
}

// Handle QuantumStarted event
void AudioInput::audioGraph_QuantumStarted(AudioGraph const& sender, IInspectable const args)
{
	auto lock = Lock();
	AudioFrame frame = frameOutputNode.GetFrame();
	AudioBuffer buffer = frame.LockBuffer(AudioBufferAccessMode::Read);
	IMemoryBufferReference reference = buffer.CreateReference();

	unsigned char* byte = nullptr;
	unsigned int capacity = 0;

	// Get the pointer to recorded data
	com_ptr<IMemoryBufferByteAccess> byteAccess = reference.as<IMemoryBufferByteAccess>();
	byteAccess->GetBuffer(&byte, &capacity);

	// Fill the audioBlock with recorded audio data
	audioBlock.insert(audioBlock.end(), reinterpret_cast<sample*>(byte), reinterpret_cast<sample*>(byte + buffer.Length()));
}

AudioInput::AudioInput() : audioGraph{ nullptr }, audioSettings{ nullptr }, inputDevice{ nullptr }, frameOutputNode{ nullptr }
{

}

AudioInput::~AudioInput()
{
	audioGraph.Close();
	audioBlock.clear();
}