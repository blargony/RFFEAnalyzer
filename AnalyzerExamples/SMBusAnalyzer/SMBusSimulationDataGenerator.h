#ifndef SMBUS_SIMULATION_DATA_GENERATOR_H
#define SMBUS_SIMULATION_DATA_GENERATOR_H

#include <AnalyzerHelpers.h>

#include "SMBusTypes.h"

class SMBusAnalyzerSettings;

class SMBusSimulationDataGenerator
{
public:
	SMBusSimulationDataGenerator();
	~SMBusSimulationDataGenerator();

	void Initialize(U32 simulation_sample_rate, SMBusAnalyzerSettings* settings);
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

protected:
	SMBusAnalyzerSettings*	mSettings;
	U32						mSimulationSampleRateHz;

	void AdvanceAllBySec(double sec)
	{
		mSMBSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(sec));
	}

	void CreateSMBusTransaction();

	void OutputStart();
	void OutputStop();
	void OutputBit(BitState state);
	U8 OutputByte(const U8 byte, bool is_ack = true);
	U8 OutputAddr(const U8 byte, bool is_read, bool is_ack = true);

	void OutputQuickCommand();
	void OutputSendByte(U8 byte);
	void OutputRecvByte(U8 byte);
	void OutputWriteByte(U8 command, U8 data_byte);
	void OutputReadByte(U8 command, U8 data_byte);
	void OutputWriteWord(U8 command, U16 data_word);
	void OutputReadWord(U8 command, U16 data_word);

	// PMBus specific
	void OutputProcessCallPMBusCoefficients();
	void OutputProcessCallPMBusRevision();
	void OutputBlockProcessCallPMBusQuery();
	void OutputPMBusGroupCommand();

protected:

	ClockGenerator mClockGenerator;

	SimulationChannelDescriptorGroup	mSMBSimulationChannels;
	SimulationChannelDescriptor*		mSMBDAT;
	SimulationChannelDescriptor*		mSMBCLK;
};

#endif	// SMBUS_SIMULATION_DATA_GENERATOR_H
