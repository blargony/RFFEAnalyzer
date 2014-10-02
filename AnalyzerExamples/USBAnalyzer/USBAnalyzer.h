#ifndef USB_ANALYZER_H
#define USB_ANALYZER_H

#include <Analyzer.h>

#include "USBAnalyzerSettings.h"
#include "USBAnalyzerResults.h"
#include "USBSimulationDataGenerator.h"

#include "USBTypes.h"

class USBAnalyzer : public Analyzer
{
public:
	USBAnalyzer();
	virtual ~USBAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

	std::string GetTimeStr(U64 sample)
	{
		char time_str[128];
		AnalyzerHelpers::GetTimeString(sample, GetTriggerSample(), GetSampleRate(), time_str, sizeof(time_str));

		return time_str;
	}

protected:	// functions

	void Setup();
	void AutoDetect();

protected:	// vars

	USBAnalyzerSettings					mSettings;
	std::auto_ptr<USBAnalyzerResults>	mResults;

	AnalyzerChannelData*				mDP;
	AnalyzerChannelData*				mDM;

	USBSimulationDataGenerator			mSimulationDataGenerator;

	bool	mSimulationInitilized;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer* analyzer);

#endif	// USB_ANALYZER_H