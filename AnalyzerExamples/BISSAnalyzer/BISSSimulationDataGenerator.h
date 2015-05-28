#ifndef BISS_SIMULATION_DATA_GENERATOR
#define BISS_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>
#include <SimulationChannelDescriptor.h>
#include <string>
class BISSAnalyzerSettings;

class BISSSimulationDataGenerator
{
public:
	BISSSimulationDataGenerator();
	~BISSSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, BISSAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	BISSAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

protected:
	void CreateBiSSFrame();
	void vMaSlo(BitState Cdm, BitState Cds);

	ClockGenerator mClockGenerator;

	SimulationChannelDescriptorGroup mBiSSSimulationChannels;
	SimulationChannelDescriptor* mMa;
	SimulationChannelDescriptor* mSlo;

	SimulationChannelDescriptor mSerialSimulationData;
};
#endif //BISS_SIMULATION_DATA_GENERATOR