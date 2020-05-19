# Tuner-UWP

Guitar tuner for Universal Windows Platform, created using C++/WinRT and FFTW3 library.

Solution consists of two main parts:
- DSP project with utilities allowing window and FIR filter generation
- Tuner project with GUI tuner application

## Notes

- When LOG_ANALYSIS macro is defined in PitchAnalyzer.h, filter_log.m and analysis_log.m Matlab files are generated
	to application's LocalState directory allowing further analysis.
- Tuner project's compilation is dependant on DSP project.