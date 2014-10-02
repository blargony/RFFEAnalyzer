#ifndef ATMEL_SWI_SIMULATION_DATA_GENERATOR_H
#define ATMEL_SWI_SIMULATION_DATA_GENERATOR_H

#include <AnalyzerHelpers.h>

#include "AtmelSWITypes.h"

class AtmelSWIAnalyzerSettings;

class AtmelSWISimulationDataGenerator
{
public:
	AtmelSWISimulationDataGenerator();
	~AtmelSWISimulationDataGenerator();

	void Initialize(U32 simulation_sample_rate, AtmelSWIAnalyzerSettings* settings);
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

protected:
	AtmelSWIAnalyzerSettings*	mSettings;
	U32							mSimulationSampleRateHz;

	void CreateSWITransaction(U8 BlockSent[], U8 BlockReceived[]);

	void OutputTokenWake();
	void OutputTokenZero();
	void OutputTokenOne();
	void OutputByte(U8 byte);
	void OutputFlag(SWI_Flag flag);
	void OutputIOBlock(U8* pByte);

protected:

	ClockGenerator mClockGenerator;

	SimulationChannelDescriptorGroup	mSWISimulationChannels;

	SimulationChannelDescriptor*		mSDA;
};

#endif	// ATMEL_SWI_SIMULATION_DATA_GENERATOR_H
