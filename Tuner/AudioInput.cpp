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

namespace winrt::Tuner::implementation
{
	// Get an instance of AudioInput class
	IAsyncAction AudioInput::Initialize()
	{
		audioSettings = AudioGraphSettings(Render::AudioRenderCategory::Media);

		// Start async operation that creates new audio graph
		CreateAudioGraphResult graphCreation{ co_await AudioGraph::CreateAsync(audioSettings) };

		// Check if succesfully created
		if (graphCreation.Status() != AudioGraphCreationStatus::Success) {
			throw runtime_error("AudioGraph creation failed.");
		}
		else {
			// Get created graph
			audioGraph = graphCreation.Graph();
			// Create output node for recorded data
			frameOutputNode = audioGraph.CreateFrameOutputNode();
			// Attach callback
			audioGraph.QuantumStarted({ this, &AudioInput::audioGraph_QuantumStarted });

			// Start audio input device node creation
			CreateAudioDeviceInputNodeResult nodeCreation{ co_await audioGraph.CreateDeviceInputNodeAsync(MediaCategory::Media) };

			// Check if succesful
			if (nodeCreation.Status() != AudioDeviceNodeCreationStatus::Success) {
				throw runtime_error("AudioDeviceInputNode creation failed.");
			}
			else {
				inputDevice = nodeCreation.DeviceInputNode();
				// Input from the recording device is routed to frameOutputNode
				inputDevice.AddOutgoingConnection(frameOutputNode);
			}
		}
	}

	// Handle QuantumStarted event
	void AudioInput::audioGraph_QuantumStarted(AudioGraph const& sender, IInspectable const args)
	{
		AudioFrame frame = frameOutputNode.GetFrame();
		Media::AudioBuffer buffer = frame.LockBuffer(AudioBufferAccessMode::Read);
		IMemoryBufferReference reference = buffer.CreateReference();

		unsigned char* byte = nullptr;
		unsigned int capacity = 0;

		// Get the pointer to recorded data
		com_ptr<IMemoryBufferByteAccess> byteAccess = reference.as<IMemoryBufferByteAccess>();
		byteAccess->GetBuffer(&byte, &capacity);

		WINRT_ASSERT(byte);

		// Fill the audioBlock with recorded audio data
		auto lock{ LockAudioInputDevice() };
		audioBuffer.insert(audioBuffer.end(), reinterpret_cast<sample_t*>(byte), reinterpret_cast<sample_t*>(byte + buffer.Length()));
	}

	AudioInput::AudioInput() : audioGraph{ nullptr }, audioSettings{ nullptr }, inputDevice{ nullptr }, frameOutputNode{ nullptr }
	{

	}

	AudioInput::~AudioInput()
	{
		audioGraph.Close();
		audioBuffer.clear();
	}
}