#ifndef PS2KEYBOARD_ANALYZER_H
#define PS2KEYBOARD_ANALYZER_H

#include <Analyzer.h>
#include "PS2KeyboardAnalyzerResults.h"
#include "PS2KeyboardSimulationDataGenerator.h"
#include "PS2KeyboardAnalyzerScanCodes.h"

class PS2KeyboardAnalyzerSettings;
class ANALYZER_EXPORT PS2KeyboardAnalyzer : public Analyzer2
{
public:
	PS2KeyboardAnalyzer();
	virtual ~PS2KeyboardAnalyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();
    virtual void SetupResults();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected: //vars
	std::auto_ptr< PS2KeyboardAnalyzerSettings > mSettings;
	std::auto_ptr< PS2KeyboardAnalyzerResults > mResults;
	AnalyzerChannelData* mClock;
	AnalyzerChannelData* mData;

	PS2KeyboardSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;
	void GetNextData(U64 &starting_frame, U64 &ending_frame, bool &DeviceToHost, U64 &Payload, bool &ParityError, bool &ACKed);

};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //PS2KEYBOARD_ANALYZER_H
