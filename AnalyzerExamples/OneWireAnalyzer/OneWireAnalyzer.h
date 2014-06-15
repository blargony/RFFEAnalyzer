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

/*
const U64 SPEC_MIN_ZERO_PULSE = 60;
const U64 SPEC_OVD_MIN_ZERO_PULSE = 6;
const U64 SPEC_MAX_ONE_PULSE = 15;
const U64 SPEC_MAX_OVD_ONE_PULSE = 2;

const U64 MARGIN_INSIDE_ZERO_PULSE = 40;
const U64 MARGIN_INSIDE_OVD_ZERO_PULSE = 1;
const U64 MARGIN_OUTSIDE_ONE_PULSE = 2;
const U64 MARGIN_OUTSIDE_OVD_ONE_PULSE = 0;
*/

const U64 SPEC_SAMPLE_POINT = 19;
const U64 SPEC_OVD_SAMPLE_POINT = 3;


class OneWireAnalyzer : public Analyzer2
{
public:
	OneWireAnalyzer();
	virtual ~OneWireAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();
	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();
	//virtual U32 GetBubbleText( Channel& channel, U32 index, DisplayBase display_base, const char ** bubble_text, U32 array_size  );
	//virtual const char* GetSimpleExportText( DisplayBase display_base );
	virtual const char* GetAnalyzerName() const;
	//virtual void StartProcessing();
	virtual bool NeedsRerun();
	
protected:
	AnalyzerChannelData* mOneWire;

	std::auto_ptr< OneWireAnalyzerSettings > mSettings;
	std::auto_ptr< OneWireAnalyzerResults > mResults;

	OneWireSimulationDataGenerator mSimulationDataGenerator;
	//virtual void AnalyzerSpecificInit();

	//void RecordBubble( ChunkedArray<ResultBubble>& one_wire_bubbles, U64 sample_start, U64 sample_end, OneWireResultBubbleType type, U64 data2 = 0, U64 data3 = 0, U64 data4 = 0 );
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
/*	
	//Simulation variables and functions.
	void SimResetPacket();
	void SimWriteBit( U32 bit );
	void SimWriteByte( U32 byte );
	void SimReadRom ( U64 rom );
	void SimSkipRom ();
	void SimMatchRom( U64 rom, bool overdrive = false );
	U64 SimSearchRom( std::vector<U64>& roms );
	void SimOverdriveSkipRom();
	void SimOverdriveMatchRom( U64 rom );


	bool mSimOverdrive;
	//ChunkedArray<U64>* mOneWireTransitions;
	U64 mSimulationSampleIndex;
	*/

};
extern "C" EXPORT const char* __cdecl GetAnalyzerName();
extern "C" EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //ONE_WIRE_ANALYZER_H