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
		CreateAudioGraphResult graphCreation = co_await AudioGraph::CreateAsync(m_audioSettings);

		// Check if succesfully created
		if (graphCreation.Status() != AudioGraphCreationStatus::Success) 
		{
			co_return false;
		}
		else 
		{
			// Get created graph
			m_audioGraph = graphCreation.Graph();
			// Create output node for recorded data
			m_frameOutputNode = m_audioGraph.CreateFrameOutputNode();
			// Attach callback
			m_audioGraph.QuantumStarted({ this, &AudioInput::audioGraph_QuantumStarted });
			// Start audio input device node creation
			CreateAudioDeviceInputNodeResult nodeCreation = co_await m_audioGraph.CreateDeviceInputNodeAsync(MediaCategory::Media);

			// Check if succesful
			if (nodeCreation.Status() != AudioDeviceNodeCreationStatus::Success) 
			{
				co_return false;
			}
			else 
			{
				m_inputDevice = nodeCreation.DeviceInputNode();
				// Input from the recording device is routed to m_frameOutputNode
				m_inputDevice.AddOutgoingConnection(m_frameOutputNode);
				co_return true;
			}
		}
	}

	// Handle QuantumStarted event
	void AudioInput::audioGraph_QuantumStarted(AudioGraph const& sender, IInspectable const args)
	{
		AudioFrame frame = m_frameOutputNode.GetFrame();
		AudioBuffer buffer = frame.LockBuffer(AudioBufferAccessMode::Read);
		IMemoryBufferReference reference = buffer.CreateReference();

		unsigned char* byte = nullptr;
		unsigned int capacity = 0;

		// Get the pointer to recorded data
		com_ptr<IMemoryBufferByteAccess> byteAccess = reference.as<IMemoryBufferByteAccess>();
		byteAccess->GetBuffer(&byte, &capacity);

		WINRT_ASSERT(byte);

		auto bufferSpaceLeft = std::distance(m_current, m_last);
		
		if (bufferSpaceLeft > buffer.Length())
		{
			std::copy(reinterpret_cast<sample_t*>(byte), reinterpret_cast<sample_t*>(byte + buffer.Length()), m_current);
			std::advance(m_current, buffer.Length() / sizeof(sample_t));
		}
		else 
		{
			std::copy(reinterpret_cast<sample_t*>(byte), reinterpret_cast<sample_t*>(byte + bufferSpaceLeft), m_current);

			RunCallbackAsync();
			SwapBuffers();
		}
	}

	void AudioInput::SwapBuffers()
	{
		m_sampleBufferQueue.push(m_pSampleBuffer);
		m_pSampleBuffer = m_sampleBufferQueue.front();
		m_sampleBufferQueue.pop();
		m_first = m_current = m_pSampleBuffer->begin();
		m_last = m_pSampleBuffer->end();
	}

	AudioInput::AudioInput() : 
		m_bufferFilledCallback	{ nullptr },
		m_audioGraph			{ nullptr }, 
		m_audioSettings			{ nullptr },
		m_inputDevice			{ nullptr }, 
		m_frameOutputNode		{ nullptr }
	{
		for (SampleBuffer& buffer : m_sampleBufferArray)
		{
			m_sampleBufferQueue.push(&buffer);

			/*
				Push placeholder std::futures for async callback execution
				for each sample buffer.
			*/
			m_asyncCallbackQueue.push(CallbackFuture());
		}

		// Prepare pointers and iterators for incoming data
		m_pSampleBuffer = m_sampleBufferQueue.front();
		m_sampleBufferQueue.pop();
		m_first = m_current = m_pSampleBuffer->begin();
		m_last = m_pSampleBuffer->end();

		// Fill audio settings
		m_audioSettings = AudioGraphSettings(AudioRenderCategory::Media);
		m_audioSettings.DesiredRenderDeviceAudioProcessing(AudioProcessing::Raw);
	}
}