#ifndef RFFE_ANALYZER_H
#define RFFE_ANALYZER_H

#include <Analyzer.h>
#include "RFFEAnalyzerResults.h"
#include "RFFESimulationDataGenerator.h"

#pragma warning( push )
//warning C4275: non dll-interface class 'Analyzer2' used 
//               as base for dll-interface class 'RFFEAnalyzer'
#pragma warning( disable : 4275 )

class RFFEAnalyzerSettings;
class ANALYZER_EXPORT RFFEAnalyzer : public Analyzer2
{
public:
	RFFEAnalyzer();
	virtual ~RFFEAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested,
                                        U32 sample_rate,
                                        SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

#pragma warning( push )
    //warning C4251: 'RFFEAnalyzer::<...>' : class <...> needs to have dll-interface
    //               to be used by clients of class
#pragma warning( disable : 4251 )

protected: //vars
	std::auto_ptr< RFFEAnalyzerSettings > mSettings;
	std::auto_ptr< RFFEAnalyzerResults > mResults;
	AnalyzerChannelData* mSclk;
	AnalyzerChannelData* mSdata;

	RFFESimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
    U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
    RFFEAnalyzerResults::RffeTypeFieldType mRffeType;

protected: // functions
	void AdvanceToBeginningStartBit();
    S32 FindStartSeqCondition();
    S32 FindSlaveAddrAndCommand();
    void FindParity(bool fromCommandFrame);
    void FindDataFrame();
    void FindAddressFrame(RFFEAnalyzerResults::RffeAddressFieldSubType type);
    void FindBusPark();
    U64  GetBitStream(U32 len, AnalyzerResults::MarkerType *states);
    bool CheckClockRate();
    void DrawMarkersDotsAndStates( U32 start,
                                   U32 len,
                                   AnalyzerResults::MarkerType type,
                                   AnalyzerResults::MarkerType *states);
    BitState GetNextBit(U32 const idx, U64 *const clk, U64 *const data );
    void FillInFrame( RFFEAnalyzerResults::RffeFrameType type,
                      U64 frame_data1,
                      U64 frame_data2,
                      U64 starting_sample,
                      U64 ending_sample,
                      U32 markers_start,
                      U32 markers_len,
                      AnalyzerResults::MarkerType *states);
private:
    U64 pulse_width2;
    U64 sampleClkOffsets[16];
    U64 sampleDataOffsets[16];

    U64 gSampleCount;
    U64 gSampleNormalized;
    U64 gSampleClk[256];
    U64 gsampleData[256];

#pragma warning( pop )
};
#pragma warning( pop )

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //RFFE_ANALYZER_H
