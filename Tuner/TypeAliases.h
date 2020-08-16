#pragma once
namespace winrt::Tuner::implementation
{
	using sample_t = float;
	using complex_t = std::complex<sample_t>;
	using AudioBufferPtrPair = std::pair<sample_t*, sample_t*>;
}