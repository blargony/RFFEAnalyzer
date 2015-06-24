# Saleae RFFE v2.0 Analyzer

Saleae Analyzer for the MIPI RFFE interface.

# OSX

In OSX, in order to compile the dylib:

* Install XCode
* Open the Project File, RFFEAnalyzer.xcodeproj
* Build
* Find the output directory under 'DerivedData' to include in the Saleae -> Options -> Preferences -> Developer tab

Tested with SDK 1.1.32 and Logic 1.2.1 Beta on OSX Yosemite

# Windows

In order to compile the DLL:

* Install Microsoft Visual Studio Express 2010
* Install the Windows SDK for Win7 (to get 64 bit libraries)
* Open the project file, RFFEAnalyzer.vcxproj
* Build
* Note the output directory and include that in the Saleae -> Options -> Preferences -> Developer tab

Tested with SDK 1.1.32 and Logic 1.1.34 Beta on Windows 7


