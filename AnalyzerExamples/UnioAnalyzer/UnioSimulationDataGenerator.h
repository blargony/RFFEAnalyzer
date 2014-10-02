#ifndef UNIO_SIMULATION_DATA_GENERATOR
#define UNIO_SIMULATION_DATA_GENERATOR

#include "UnioAnalyzerSettings.h"
#include "AnalyzerHelpers.h"

class UnioSimulationDataGenerator
{
public:
	UnioSimulationDataGenerator(  );
	~UnioSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, UnioAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

protected:
	UnioAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
	SimulationChannelDescriptor mUnioSimulationData;  //if we had more than one channel to simulate, they would need to be in an array

protected: //UNIO specific
	ClockGenerator mClockGenerator;

	void WriteStandbyPulse();
	void WriteMinimumSameAddressDelay();
	void WriteHeader();

	void WriteByte( U8 data, MasterAcknowkedge master_ack, SlaveAcknowkedge slave_ack, bool valid=true, bool mak_hold=false );
	void WriteByteWithHold( U8 data, MasterAcknowkedge master_ack, SlaveAcknowkedge slave_ack );
	
	void WriteAck();
	void WriteAckWithHold();
	void WriteNak();
	void WriteNonStandardNak();
	
	void WriteBit( BitState bit_state );

protected:	//fake data generation settings
	double mBitRate;
	double mStandyPulseTimeS;
	double mStartHeaderLowPulseTimeS;
	double mHoldDelayS;
	double mTssMinimumSameAddressDelayS;

};
#endif //UNIO_SIMULATION_DATA_GENERATOR
