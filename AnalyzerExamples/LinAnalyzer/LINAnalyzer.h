#ifndef LIN_ANALYZER_H
#define LIN_ANALYZER_H

#include <Analyzer.h>
#include "LINAnalyzerResults.h"
#include "LINSimulationDataGenerator.h"
#include "LINChecksum.h"

class LINAnalyzerSettings;
class ANALYZER_EXPORT LINAnalyzer : public Analyzer
{
public:
	LINAnalyzer();
	virtual ~LINAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected:
	U8 ByteFrame( S64& startingSample, S64& endingSample, bool& framingError );
	U8 GetBreakField( S64& startingSample, S64& endingSample, bool& framingError );
	inline double SamplesPerBit();
	double HalfSamplesPerBit();

	void AdvanceHalfBit();
	void Advance(U16 nBits);

protected: //vars
	std::auto_ptr< LINAnalyzerSettings > mSettings;
	std::auto_ptr< LINAnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;

	LINSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;
	LINAnalyzerResults::tLINFrameState mFrameState;
	LINChecksum mChecksum;

};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //LIN_ANALYZER_H
