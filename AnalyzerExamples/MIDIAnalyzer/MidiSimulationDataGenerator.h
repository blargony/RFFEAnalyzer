#ifndef MIDI_SIMULATION_DATA_GENERATOR
#define MIDI_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>
class MidiAnalyzerSettings;

class MidiSimulationDataGenerator
{
public:
	MidiSimulationDataGenerator();
	~MidiSimulationDataGenerator();
	
	void Initialize( U32 simulation_sample_rate, MidiAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );
	
protected:
	MidiAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
	SimulationChannelDescriptor mMidiSimulationData;
	// Make functions to add bad data to simulation.
	void injectBurstErrors();
	
	void CreateMidiMessage();
	void loadTestData();		// Only for testing
	void loadSampleData();		// Plausible MIDI for end-user
	//std::string mSerialText;
	//const char *mSerialTextBuf;
	U32 mMidiBuf[31];
	//U32 mStringIndex;
	U32 mMidiBufIndex;
	U32 mMidiBufShortEnd;
};

#endif //MIDI_SIMULATION_DATA_GENERATOR
