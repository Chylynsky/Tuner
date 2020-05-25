#include "pch.h"
#include "AudioInput.h"

using namespace winrt;
using namespace winrt::Windows;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Media::Audio;
using namespace winrt::Windows::Media::Capture;
using namespace winrt::Windows::Media::MediaProperties;
using namespace winrt::Windows::Media::Render;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Devices::Enumeration;

namespace winrt::Tuner::implementation
{
	// Get an instance of AudioInput class
	std::future<AudioInputInitializationStatus> AudioInput::InitializeAsync()
	{
		audioSettings = AudioGraphSettings(AudioRenderCategory::Other);
		// Start async operation that creates new audio graph
		CreateAudioGraphResult graphCreation{ co_await AudioGraph::CreateAsync(audioSettings) };

		// Check if succesfully created
		if (graphCreation.Status() != AudioGraphCreationStatus::Success) {
			co_return AudioInputInitializationStatus::Failure; // throw runtime_error("AudioGraph creation failed.");
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
				co_return AudioInputInitializationStatus::Failure; // throw runtime_error("AudioDeviceInputNode creation failed.");
			}
			else {
				inputDevice = nodeCreation.DeviceInputNode();
				// Input from the recording device is routed to frameOutputNode
				inputDevice.AddOutgoingConnection(frameOutputNode);
				co_return AudioInputInitializationStatus::Success;
			}
		}
	}

	// Handle QuantumStarted event
	void AudioInput::audioGraph_QuantumStarted(AudioGraph const& sender, IInspectable const args)
	{
		AudioFrame frame = frameOutputNode.GetFrame();
		AudioBuffer buffer = frame.LockBuffer(AudioBufferAccessMode::Read);
		IMemoryBufferReference reference = buffer.CreateReference();

		unsigned char* byte = nullptr;
		unsigned int capacity = 0;

		// Get the pointer to recorded data
		com_ptr<IMemoryBufferByteAccess> byteAccess = reference.as<IMemoryBufferByteAccess>();
		byteAccess->GetBuffer(&byte, &capacity);

		WINRT_ASSERT(byte);
		
		if (std::next(current, buffer.Length()) < last) {
			std::copy(reinterpret_cast<float*>(byte), reinterpret_cast<float*>(byte + buffer.Length()), current);
			//std::memcpy(current, byte, buffer.Length());
			std::advance(current, buffer.Length() / sizeof(float));
		}
		else {
			auto distance = std::distance(current, last);
			std::copy(reinterpret_cast<float*>(byte), reinterpret_cast<float*>(byte + distance), current);
			//std::memcpy(current, byte, distance);
			std::advance(current, distance / sizeof(float));
			bufferFilledCallback(*this, audioBufferIters);
		}
	}

	AudioInput::AudioInput() : 
		bufferFilledCallback{ nullptr },
		audioGraph{ nullptr }, 
		audioSettings{ nullptr }, 
		inputDevice{ nullptr }, 
		frameOutputNode{ nullptr }, 
		first{ nullptr },
		current{ nullptr },
		last{ nullptr }
	{
	}
}