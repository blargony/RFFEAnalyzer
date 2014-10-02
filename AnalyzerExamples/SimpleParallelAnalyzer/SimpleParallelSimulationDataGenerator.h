#ifndef SIMPLEPARALLEL_SIMULATION_DATA_GENERATOR
#define SIMPLEPARALLEL_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <vector>
class SimpleParallelAnalyzerSettings;

class SimpleParallelSimulationDataGenerator
{
public:
	SimpleParallelSimulationDataGenerator();
	~SimpleParallelSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, SimpleParallelAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	SimpleParallelAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
	U16 mValue;

protected:

	SimulationChannelDescriptorGroup mSimulationData;

	std::vector< SimulationChannelDescriptor* > mData;
	std::vector< U16 > mDataMasks;
	SimulationChannelDescriptor* mClock;
};
#endif //SIMPLEPARALLEL_SIMULATION_DATA_GENERATOR
