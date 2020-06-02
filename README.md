# Tuner-UWP

Guitar tuner for Universal Windows Platform, created using C++/WinRT and FFTW3 library.

Solution consists of two main parts:
- DSP project with utilities allowing window and FIR filter generation
- Tuner project with GUI tuner application

## Notes

- During the first app launch, loading may take a while. This is due to the FFTW best peformant algorithm calculation. The result of
	these calculations is saved locally and loaded in the next app launches.
- Pitch detection is performed using a Harmonic Product Spectrum algorithm.
- When *CREATE_MATLAB_PLOTS* macro is defined in *PitchAnalyzer.h*, *filter_log.m* and *analysis_log.m* Matlab files are generated
	to application's *LocalState* directory allowing further inspection.
- Best way to find these files is to search for them in *C:\Users\username\AppData* (AppData is a hidden folder)
- *Tuner* project's compilation is dependant on *DSP* project.

## Screenshots

Several views of the main page:

![Main page](/Screenshots/app_main_page1.png)

![Main page](/Screenshots/app_main_page2.png)

![Main page](/Screenshots/app_main_page3.png)

Generated FIR filter based on Blackman-Harris window (plot from auto-generated *filter_log.m* Matlab file using CREATE_MATLAB_PLOTS macro):

![FIR filter](/Screenshots/filter.png)

FIR filter closeup:

![FIR filter closeup](/Screenshots/filter_closeup.png)

Example of Matlab plot from auto-generated *analysis_log.m* Matlab file using CREATE_MATLAB_PLOTS macro, screenshot
shows the signal after applying FIR filter, taken while playing high E guitar string:

![Filtered signal](/Screenshots/filtered.png)

Filtered signal closeup:

![Filtered signal closeup](/Screenshots/filtered_closeup.png)

## Author
* **Borys Chyliński** - [Chylynsky](https://github.com/Chylynsky)