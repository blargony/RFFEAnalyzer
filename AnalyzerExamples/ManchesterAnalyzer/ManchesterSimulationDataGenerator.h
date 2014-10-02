#ifndef MANCHESTER_SIMULATION_DATA_GENERATOR
#define MANCHESTER_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>

class ManchesterAnalyzerSettings;

class ManchesterSimulationDataGenerator
{
public:
	ManchesterSimulationDataGenerator();
	~ManchesterSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, ManchesterAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

protected:

	U64 UsToSamples( U64 us );
	U64 UsToSamples( double us );
	U64 SamplesToUs( U64 samples );

	void SimWriteByte( U64 value );
	void SimWriteBit( U32 bit );

	U64 mT;
	U64 mSimValue;

	ManchesterAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

	SimulationChannelDescriptor mManchesterSimulationData; 
};

#endif //MANCHESTER_SIMULATION_DATA_GENERATOR
