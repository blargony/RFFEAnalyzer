#ifndef SMBUS_ANALYZER_H
#define SMBUS_ANALYZER_H

#include <Analyzer.h>

#include "SMBusAnalyzerSettings.h"
#include "SMBusAnalyzerResults.h"
#include "SMBusSimulationDataGenerator.h"

#include "SMBusTypes.h"

class SMBusAnalyzer : public Analyzer
{
public:
	SMBusAnalyzer();
	virtual ~SMBusAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

	// helpers
	AnalyzerChannelData* GetNearestTransitionChannel();
	AnalyzerChannelData* AdvanceAllTo(U64 toSample);

	// returns true if it state is a Zero or a One, false for Start or stop
	bool GetSignal(SMBusSignalState& state);

protected:	// functions

	void Setup();

protected:	// vars

	SMBusAnalyzerSettings				mSettings;
	std::auto_ptr<SMBusAnalyzerResults>	mResults;

	AnalyzerChannelData*				mSMBDAT;
	AnalyzerChannelData*				mSMBCLK;

	SMBusSimulationDataGenerator		mSimulationDataGenerator;

	bool	mSimulationInitilized;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer* analyzer);

#endif	// SMBUS_ANALYZER_H