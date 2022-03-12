cmake_minimum_required (VERSION 3.13)

project(RffeAnalyzer)

add_definitions( -DLOGIC2 )

# enable generation of compile_commands.json, helpful for IDEs to locate include files.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/RffeUtil.cpp
src/RffeUtil.h
src/RffeAnalyzer.cpp
src/RffeAnalyzer.h
src/RffeAnalyzerResults.cpp
src/RffeAnalyzerResults.h
src/RffeAnalyzerSettings.cpp
src/RffeAnalyzerSettings.h
src/RffeSimulationDataGenerator.cpp
src/RffeSimulationDataGenerator.h
)

add_analyzer_plugin(${PROJECT_NAME} SOURCES ${SOURCES})
