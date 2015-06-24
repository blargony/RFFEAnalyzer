# Saleae RFFE v2.0 Analyzer

Saleae Analyzer for the MIPI RFFE interface.

# OSX
In OSX, in order to compile the dylib:
* Install XCode
* Open the Project File, RFFEAnalyzer.xcodeproj
* Build
* Find the output directory under 'DerivedData' to include in the Saleae -> Options -> Preferences -> Developer tab

## OSX Build Details
* Tested with Saleae Analyzer SDK 1.1.32 and Logic 1.2.1 Beta on OSX Yosemite
* XCode Version 6.3.2 (6D2105)

# Windows
In order to compile the DLL:
* Install Microsoft Visual Studio Express 2010
* Install the Windows SDK for Win7 (to get 64 bit libraries)
* Open the project file, RFFEAnalyzer.vcxproj
* Build (F7)
* Note the output directory and include that in the Saleae -> Options -> Preferences -> Developer tab

## Windows Build Details
  * Tested with Saleae Analyzer SDK 1.1.32 and Logic 1.1.34 Beta on Windows 7
  * Microsoft Visual Studio 2010 - Version 10.0.40219.1 SP1Rel
  * Microsoft .NET Framework - Version 4.5.51209 SP1Rel
  * Installed Products
    * Microsoft Visual C++ 2010   01013-169-2610014-70651
    * Hotfix for Microsoft Visual C++ 2010 Express - ENU (KB2542054)   KB2542054
    * Hotfix for Microsoft Visual C++ 2010 Express - ENU (KB2635973)   KB2635973
    * Microsoft Visual C++ 2010 Express - ENU Service Pack 1 (KB983509)   KB983509
