#ifndef ATMEL_SWI_ANALYZER_H
#define ATMEL_SWI_ANALYZER_H

#include <Analyzer.h>

#include "AtmelSWIAnalyzerSettings.h"
#include "AtmelSWIAnalyzerResults.h"
#include "AtmelSWISimulationDataGenerator.h"

#include "AtmelSWITypes.h"

enum AtmelSWIFrameType
{
	FrameToken,
	FrameByte,
	FrameFlag,
	FrameCount,
	FrameChecksum,

	FramePacketSegment,
};

class WakeException: public std::exception {};

class AtmelSWIAnalyzer : public Analyzer2
{
public:
	AtmelSWIAnalyzer();
	virtual ~AtmelSWIAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);
	virtual U32 GetMinimumSampleRateHz();
    virtual void SetupResults();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected:	// functions

	void Setup();

	void AddFrame(U64 SampleBegin, U64 SampleEnd, AtmelSWIFrameType FrameType, U64 Data1 = 0, U64 Data2 = 0);

	void ParsePacket(const SWI_Block& block, size_t block_ndx, const std::vector<std::pair<U64, U64> >& ByteSamples);

	// reads raw bytes, makes frames out of them and throws a WakeException on the first wake token
	void ResyncToWake(SWI_WaveParser& tokenizer);

protected:	// vars

	AtmelSWIAnalyzerSettings				mSettings;
	std::auto_ptr<AtmelSWIAnalyzerResults>	mResults;

	AnalyzerChannelData*					mSDA;

	AtmelSWISimulationDataGenerator			mSimulationDataGenerator;

	bool mSimulationInitilized;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer* analyzer);

#endif	// ATMEL_SWI_ANALYZER_H
