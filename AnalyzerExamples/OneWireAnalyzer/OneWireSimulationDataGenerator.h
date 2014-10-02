#ifndef ONE_WIRE_SIMULATION_DATA_GENERATOR
#define ONE_WIRE_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>

class OneWireAnalyzerSettings;

class OneWireSimulationDataGenerator
{
public:
	OneWireSimulationDataGenerator();
	~OneWireSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, OneWireAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

protected:
	OneWireAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected: //Serial specific
	
	U64 UsToSamples( U64 us );
	U64 SamplesToUs( U64 samples );

	//Simulation variables and functions.
	void SimResetPacket();
	void SimWriteBit( U32 bit );
	void SimWriteByte( U32 byte );
	void SimReadRom ( U64 rom );
	void SimSkipRom ();
	void SimMatchRom( U64 rom, bool overdrive = false );
	U64 SimSearchRom( std::vector<U64>& roms );
	void SimOverdriveSkipRom();
	void SimOverdriveMatchRom( U64 rom );


	bool mSimOverdrive;
	U64 mSimulationSampleIndex;

	SimulationChannelDescriptor mOneWireSimulationData;  //if we had more than one channel to simulate, they would need to be in an array
};
#endif //ONE_WIRE
