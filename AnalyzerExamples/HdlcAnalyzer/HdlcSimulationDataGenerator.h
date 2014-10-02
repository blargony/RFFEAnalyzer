#ifndef HDLC_SIMULATION_DATA_GENERATOR
#define HDLC_SIMULATION_DATA_GENERATOR

#include <SimulationChannelDescriptor.h>
#include "HdlcAnalyzerSettings.h"
#include <string>
#include <vector>

using namespace std;

class HdlcSimulationDataGenerator
{
public:
	HdlcSimulationDataGenerator();
	~HdlcSimulationDataGenerator();

	void Initialize ( U32 simulation_sample_rate, HdlcAnalyzerSettings* settings );
	U32 GenerateSimulationData ( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

	static vector<U8> Crc8 ( const vector<U8> & stream );
	static vector<U8> Crc16 ( const vector<U8> & stream );
	static vector<U8> Crc32 ( const vector<U8> & stream );
	static vector<U8> CrcDivision ( const vector<U8> & stream, const vector<U8> & genPoly, U32 crcNumber );
	static vector<BitState> BytesVectorToBitsVector ( const vector<U8> & v, U32 numberOfBits );

protected:

	void CreateHDLCFrame ( const vector<U8> & address, const vector<U8> & control, const vector<U8> & information );
	void CreateFlag();

	// Sync Transmission
	void CreateFlagBitSeq();
	void CreateSyncBit ( BitState bitState );
	void TransmitBitSync ( const vector<U8> & stream );

	// Async transmission
	void TransmitByteAsync ( const vector<U8> & stream );
	void CreateAsyncByte ( U8 byte );
	void AsyncByteFill ( U32 N );

	// Helper functions
	bool AbortFrameAndGenIndex ( U32 N );
	vector<U8> GenFcs ( HdlcFcsType fcsType, const vector<U8> & stream ) const;

	void GenerateAbortFramesIndexes();
	bool ContainsElement ( U32 index ) const;

	vector<U8> GenAddressField ( HdlcAddressType addressType, U64 addressBytes, U8 value ) const;
	vector<U8> GenControlField ( HdlcFrameType frameType, HdlcControlType controlType, U8 value ) const;
	vector<U8> GenInformationField ( U16 size, U8 value ) const;

	HdlcAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;

	vector<U32> mAbortFramesIndexes;
	U32 mFrameNumber;
	U32 mAbortByte;
	U32 mWrongFramesSeparation;

	U8 mControlValue;
	U8 mAddresByteValue;
	U8 mInformationByteValue;

	HdlcFrameType mFrameTypes[ 3 ];

	SimulationChannelDescriptor mHdlcSimulationData;

	U64 mSamplesInHalfPeriod;
	U64 mSamplesInAFlag;

};
#endif //HDLC_SIMULATION_DATA_GENERATOR