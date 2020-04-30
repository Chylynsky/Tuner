# Tuner-UWP
Guitar Tuner for Universal Windows Platform, created using C++/WinRT.

In short about files in the project:

1. MainPage.h, MainPage.cpp specify the bahaviour of the application from the user's point of view (visual representation of th results), 
  as well as bind all the components together.
2. PitchAnalyer.h, PitchAnalyzer.cpp use FFTW library to perform the sound harmonic analysis and calculate the note with the 
  highest amplitude.
3. AudioInput.h, AudioInput.cpp connect to audio input device choosen currently in the systems settings and store the recorded audio 
  samples in a buffer.
