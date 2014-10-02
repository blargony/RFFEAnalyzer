#ifndef MODBUS_ANALYZER_H
#define MODBUS_ANALYZER_H

#include <Analyzer.h>
#include "ModbusAnalyzerResults.h"
#include "ModbusSimulationDataGenerator.h"
#include "ModbusAnalyzerModbusExtension.h"

#include <stdio.h>
#include <string.h>

class ModbusAnalyzerSettings;
class ANALYZER_EXPORT ModbusAnalyzer : public Analyzer2
{
public:
	ModbusAnalyzer();
	virtual ~ModbusAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();
	


#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'ModbusAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class

protected: //functions
	void ComputeSampleOffsets();
	U64 GetNextByteModbus(U32 num_bits, U64 bit_mask, U64 &frame_starting_sample, U64 &frame_ending_sample);
	int ASCII2INT(char value);

protected: //vars
	std::auto_ptr< ModbusAnalyzerSettings > mSettings;
	std::auto_ptr< ModbusAnalyzerResults > mResults;
	AnalyzerChannelData* mModbus;

	ModbusSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Modbus analysis vars:
	U32 mSampleRateHz;
	std::vector<U32> mSampleOffsets;
	U32 mParityBitOffset;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
	BitState mBitLow;
	BitState mBitHigh;

	//Checksum caluclations for Modbus
	U16   crc_tab16[256];
	void init_crc16_tab( void );
	U16 update_CRC( U16 crc, U8 c );

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //MODBUS_ANALYZER_H
