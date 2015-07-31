# Saleae RFFE v2.0 Analyzer

Saleae Analyzer for the MIPI RFFE interface.

# OSX
In OSX, in order to compile the dylib:
* Install XCode
* Open the Project File, RFFEAnalyzer.xcodeproj
* Build
* Find the output directory under 'DerivedData' to include in the Saleae -> Options -> Preferences -> Developer tab

## OSX Build Details
* Tested with Saleae Analyzer SDK 1.1.32 and Logic 1.2.2 Beta on OSX Yosemite
* XCode Version 6.3.2 (6D2105)

# Windows
In order to compile the DLL:
* Install Microsoft Visual Studio Community 2015 (with Visual C++)
* Open the project file, RFFEAnalyzer.vcxproj
* Build (F7)
* Note the output directory and include that in the Saleae -> Options -> Preferences -> Developer tab

## Windows Build Details
  * Tested with Saleae Analyzer SDK 1.2.3 Beta on Windows 10
  * Microsoft Visual Community 2015 - Version 14.0.23107.0 D14REL
  * Microsoft .NET Framework - Version 4.6.00079
  * Microsoft Visual C++ 2015 - 00322-20000-00000-AA121
