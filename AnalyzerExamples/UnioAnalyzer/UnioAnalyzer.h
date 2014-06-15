#ifndef UNIO_ANALYZER_H
#define UNIO_ANALYZER_H


#include <Analyzer.h>
#include "UnioAnalyzerResults.h"
#include "UnioSimulationDataGenerator.h"

class UnioAnalyzerSettings;

enum UnioHeaderResult { HeaderValid, HeaderInvalid };
enum UnioBitResult { High, Low, Invalid };
enum UnioByteResult { ByteValid, ByteInvalid };
enum UnioTransactionResult { TransactionNormal, TransactionError };
enum UnioSlaveAcknowledge { Sak, NoSak };
enum UnioMasterAcknowledge { Mak, NoMak };

class UnioAnalyzer : public Analyzer2
{
public:
	UnioAnalyzer();
	virtual ~UnioAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'SerialAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class

protected:  //functions
	void GetTransaction();
	UnioByteResult GetAddress();
	void MoveToFallingEdgeOfStandbyPulse();
	void MoveToNextStartHeaderAfterNoMakSak();
	UnioByteResult GetByte( U8& data, UnioMasterAcknowledge& master_ack, UnioSlaveAcknowledge& slave_ack );
	UnioHeaderResult GetHeader();
	UnioBitResult GetBit( bool allow_mak_hold = false );
	UnioBitResult GetMak();
	UnioSlaveAcknowledge GetSak();

	void AdvanceToNextEdgeIgnoreSpikes();
	U32 GetNumEdgesBeforeSampleIgnoreSpikes( U64 sample );

protected:  //vars
	std::auto_ptr< UnioAnalyzerSettings > mSettings;
	std::auto_ptr< UnioAnalyzerResults > mResults;
	AnalyzerChannelData* mScio;

	UnioSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	U32 mSampleRateHz;

	//state
	U64 mBitStartingSample;

	//constants:
	double mStandyPulseTimeS;
	U32 mStandyPulseSamples;

	double mInputSpikeSuppressionTimeS;
	U32 mInputSpikeSuppressionSamples;

	//computed constants:
	U32 mSamplesPerBit;
	U32 mSamplesPerHalfBit;
	U32 mSamplesPerQuarterBit;
	U32 mSamplesPerThreeQuartersBit;
	double mBitRate;
	double mTimePerSample;

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //UNIO_ANALYZER_H