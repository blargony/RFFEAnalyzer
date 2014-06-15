#ifndef DMX512_SIMULATION_DATA_GENERATOR
#define DMX512_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <AnalyzerHelpers.h>
#include <string>
class Dmx512AnalyzerSettings;

class Dmx512SimulationDataGenerator
{
public:
	Dmx512SimulationDataGenerator();
	~Dmx512SimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, Dmx512AnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	ClockGenerator mClockGenerator;
	Dmx512AnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
	int mSlots;

protected:
	SimulationChannelDescriptor mSerialSimulationData;
	U32 BreakLength( void );
	U32 MarkLength( void );
	U32 MBBLength( void );
	U32 MABLength( void );
};
#endif //DMX512_SIMULATION_DATA_GENERATOR
