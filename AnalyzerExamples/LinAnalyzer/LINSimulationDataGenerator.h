#ifndef LIN_SIMULATION_DATA_GENERATOR
#define LIN_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include <string>

#include "LINChecksum.h"

class LINAnalyzerSettings;

class LINSimulationDataGenerator
{
public:
	LINSimulationDataGenerator();
	~LINSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, LINAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
	void CreateFrame();
	void CreateHeader();
	void CreateReponse( U8 length );
	void CreateBreakField();
	void CreateSyncField();
	void CreateProtectedIdentifierField( U8 id );
	void CreateSerialByte( U8 byte );
	void SwapEnds( U8& byte );
	U32 Random(  U32 min, U32 max );

private:
	LINAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
	SimulationChannelDescriptor mSerialSimulationData;
	LINChecksum mChecksum;

};
#endif //LIN_SIMULATION_DATA_GENERATOR
