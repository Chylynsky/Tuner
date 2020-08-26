#include "pch.h"
#include "AudioInput.h"

using namespace winrt;
using namespace winrt::Windows;
using namespace winrt::Windows::Foundation;
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
	IAsyncOperation<bool> AudioInput::InitializeAsync()
	{
		// Start async operation that creates new audio graph
		CreateAudioGraphResult graphCreation = co_await AudioGraph::CreateAsync(audioSettings);

		// Check if succesfully created
		if (graphCreation.Status() != AudioGraphCreationStatus::Success) 
		{
			co_return false;
		}
		else 
		{
			// Get created graph
			audioGraph = graphCreation.Graph();
			// Create output node for recorded data
			frameOutputNode = audioGraph.CreateFrameOutputNode();
			// Attach callback
			audioGraph.QuantumStarted({ this, &AudioInput::audioGraph_QuantumStarted });
			// Start audio input device node creation
			CreateAudioDeviceInputNodeResult nodeCreation = co_await audioGraph.CreateDeviceInputNodeAsync(MediaCategory::Media);

			// Check if succesful
			if (nodeCreation.Status() != AudioDeviceNodeCreationStatus::Success) 
			{
				co_return false;
			}
			else 
			{
				inputDevice = nodeCreation.DeviceInputNode();
				// Input from the recording device is routed to frameOutputNode
				inputDevice.AddOutgoingConnection(frameOutputNode);
				co_return true;
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
		
		if (current + buffer.Length() < last) {
			std::copy(reinterpret_cast<sample_t*>(byte), reinterpret_cast<sample_t*>(byte + buffer.Length()), current);
			current += buffer.Length() / sizeof(sample_t);
		}
		else {
			auto distance = last - current;
			std::copy(reinterpret_cast<sample_t*>(byte), reinterpret_cast<sample_t*>(byte + distance), current);
			current += buffer.Length() / sizeof(sample_t);

			RunCallbackAsync(first, current);
			SwapBuffers();
		}
	}

	void AudioInput::SwapBuffers()
	{
		sampleBufferQueue.push(sampleBufferPtr);
		sampleBufferPtr = sampleBufferQueue.front();
		sampleBufferQueue.pop();
		first = current = sampleBufferPtr->data();
		last = first + sampleBufferPtr->size();
	}

	AudioInput::AudioInput() : 
		bufferFilledCallback{ nullptr },
		audioGraph{ nullptr }, 
		audioSettings{ nullptr },
		inputDevice{ nullptr }, 
		frameOutputNode{ nullptr }
	{
		for (SampleBuffer& buffer : sampleBufferArray)
		{
			sampleBufferQueue.push(&buffer);

			/*
				Push placeholder std::futures for async callback execution
				for each sample buffer.
			*/
			asyncCallbackQueue.push(std::future<void>());
		}

		sampleBufferPtr = sampleBufferQueue.front();
		sampleBufferQueue.pop();
		first = current = sampleBufferPtr->data();
		last = first + sampleBufferPtr->size();

		// Fill audio settings
		audioSettings = AudioGraphSettings(AudioRenderCategory::Media);
		audioSettings.DesiredRenderDeviceAudioProcessing(AudioProcessing::Raw);
	}
}