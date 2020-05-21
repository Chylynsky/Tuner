# Tuner-UWP

Guitar tuner for Universal Windows Platform, created using C++/WinRT and FFTW3 library.

Solution consists of two main parts:
- DSP project with utilities allowing window and FIR filter generation
- Tuner project with GUI tuner application

## Notes

- When LOG_ANALYSIS macro is defined in PitchAnalyzer.h, filter_log.m and analysis_log.m Matlab files are generated
	to application's LocalState directory allowing further inspection.
- Best way to find these files is to search for them (AppData is a hidden folder) in C:\Users\{username}\AppData
- Tuner project's compilation is dependant on DSP project.

## Screenshots

### Main page:


![Main page](/images/app_main_page.png)


### Generated FIR filter based on Blackman-Harris window (plot from auto-generated Matlab file using LOG_ANALYSIS macro):


![FIR filter](/images/fir_filter1.png)


### FIR filter closeup:


![FIR filter closeup](/images/fir_filter2.png)


### Example of auto-generated Matlab code:


![Auto-generated code 1](/images/analysis_log_example.png)