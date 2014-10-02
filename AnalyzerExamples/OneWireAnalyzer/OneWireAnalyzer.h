#ifndef ONE_WIRE_ANALYZER_H
#define ONE_WIRE_ANALYZER_H

#ifdef WIN32
	#define EXPORT __declspec(dllexport)
#else
	#define EXPORT
	#define __cdecl
	#define __stdcall
	#define __fastcall
#endif

#include "Analyzer.h"
#include "OneWireAnalyzerResults.h"
#include "OneWireSimulationDataGenerator.h"

class OneWireAnalyzerSettings;

enum OneWireFrameType { RestartPulse, PresencePulse, ReadRomFrame, SkipRomFrame, SearchRomFrame, MatchRomFrame, OverdriveSkipRomFrame, OverdriveMatchRomFrame, CRC, FamilyCode, Rom, Byte, Bit, InvalidRomCommandFrame };
enum OneWireState { UnknownState, ResetDetectedState, PresenceDetectedState, RomCommandDetectedState, RomFinishedState };
enum OneWireRomCommand { ReadRom, SkipRom, SearchRom, MatchRom, OverdriveSkipRom, OverdriveMatchRom };

//http://www.maxim-ic.com/products/ibutton/ibuttons/standard.pdf
const U64 SPEC_RESET_PULSE = 480;
const U64 SPEC_OVD_RESET_PULSE = 48;

const U64 MARGIN_INSIDE_RESET_PULSE = 80;
const U64 MARGIN_INSIDE_OVD_RESET_PULSE = 8;

const U64 SPEC_MIN_PRESENCE_PULSE = 60;
const U64 SPEC_MIN_OVD_PRESENCE_PULSE = 8;
const U64 SPEC_MAX_PRESENCE_PULSE = 240;
const U64 SPEC_MAX_OVD_PRESENCE_PULSE = 24;

const U64 MARGIN_INSIDE_PRESENCE_PULSE = 10;
const U64 MARGIN_INSIDE_OVD_PRESENCE_PULSE = 1;
const U64 MARGIN_OUTSIDE_PRESENCE_PULSE = 10;
const U64 MARGIN_OUTSIDE_OVD_PRESENCE_PULSE = 1;



const U64 SPEC_SAMPLE_POINT = 19;
const U64 SPEC_OVD_SAMPLE_POINT = 3;


class ANALYZER_EXPORT OneWireAnalyzer : public Analyzer2
{
public:
	OneWireAnalyzer();
	virtual ~OneWireAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();
	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;

	virtual bool NeedsRerun();
	
#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'ManchesterAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class
protected:
	AnalyzerChannelData* mOneWire;

	std::auto_ptr< OneWireAnalyzerSettings > mSettings;
	std::auto_ptr< OneWireAnalyzerResults > mResults;

	OneWireSimulationDataGenerator mSimulationDataGenerator;

	void RecordFrame( U64 starting_sample, U64 ending_sample, OneWireFrameType type, U64 data = 0, bool warning = false );
	U64 UsToSamples( U64 us );
	U64 SamplesToUs( U64 samples );

	U32 mSampleRateHz;

	//decoding variables and functions.
	bool mOverdrive;						//1-Wire in Overdrive mode. approx speed increase.
	OneWireState mCurrentState;				//Current state comunication is in: Reset/Romcommand/Rom data (for Match, Skip and Read)/Transfers
	OneWireRomCommand mCurrentRomCommand;	//stores which rom command was issued.
	U32 mRomBitsRecieved;					//count of ROM bits recieved after a rom command.
	U64 mRomDetected;						//actual rom of device detected.
	U64 mDataDetected;						//data from device. either a rom command or general data.
	U32 mDataBitsRecieved;					//count of data bits recieved.

	U64 mByteStartSample;

	U64 mRisingEdgeSample;
	U64 mFallingEdgeSample;
	U64 mPreviousRisingEdgeSample;
	U64 mLowPulseLength;					//units - samples
	U64 mHighPulseLength;					//units - samples
	U64 mLowPulseTime;						//units - microseconds us 10^-6
	U64 mHighPulseTime;						//units - microseconds us 10^-6
	bool mBlockPulseAdvance;				//in the while loop, continue without advancing the transision!

	bool mSimulationInitilized;

#pragma warning( pop )
};
extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName( );
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //ONE_WIRE_ANALYZER_H
