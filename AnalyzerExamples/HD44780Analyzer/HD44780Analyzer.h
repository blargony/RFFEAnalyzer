#ifndef HD44780_ANALYZER_H
#define HD44780_ANALYZER_H

#include <Analyzer.h>
#include "HD44780AnalyzerResults.h"
#include "HD44780SimulationDataGenerator.h"

class HD44780AnalyzerSettings;
class ANALYZER_EXPORT HD44780Analyzer : public Analyzer2
{
public:
	HD44780Analyzer();
	virtual ~HD44780Analyzer();
	virtual void WorkerThread();

    virtual void SetupResults();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< HD44780AnalyzerSettings > mSettings;
	std::auto_ptr< HD44780AnalyzerResults > mResults;

	AnalyzerChannelData *mE, *mRS, *mRW, *mDB[8];

	HD44780SimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

  U64 mLastEStart,mWaitBusy;
	U32 mSampleRateHz;
  bool bitmode8;

  U32 TimeToSamplesOrMore(double AS);
  U32 TimeToSamplesOrLess(double AS);
  void GetTransfer();
  bool GetOperation(bool &ARS, bool &ARW, U8 &AData, S64 &AEStart, S64 &AEEnd, bool ASecondNibble);
  void AdvanceToAbsPositionWhileMarking(AnalyzerChannelData *AACD, Channel AC, U64 APosition);
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#define MFLAG_RS 0
#define MFLAG_RW 1

#endif //HD44780_ANALYZER_H
