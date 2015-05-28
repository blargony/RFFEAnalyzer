#ifndef MIDI_ANALYZER_H
#define MIDI_ANALYZER_H

#include <Analyzer.h>
#include "MidiAnalyzerResults.h"
#include "MidiSimulationDataGenerator.h"

class MidiAnalyzerSettings;
class ANALYZER_EXPORT MidiAnalyzer : public Analyzer2
{
public:
	MidiAnalyzer();
	virtual ~MidiAnalyzer();
	virtual void WorkerThread();
	
	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();
	
	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();
	
    virtual void SetupResults();
protected: //vars
	union stash {
		U64 mData2;
		S8 channel;
	} stash1;
	bool inCommandPacket;
	S64 distanceFromLastCommandPacket;
	S32 dataFrameCorrespondingChannel;
	enum MidiFrameType lastCommandPacketType;
	
protected:	//functions
	void dataParameter( U8 mData, enum MidiFrameType *mFrameType, union stash *mStash1);
	void systemMessage( U8 mData, enum MidiFrameType *mFrameType, union stash *mStash1);
	void channelMessage( U8 mData, enum MidiFrameType *mFrameType, union stash *mStash1);

protected: //vars
	std::auto_ptr< MidiAnalyzerSettings > mSettings;
	std::auto_ptr< MidiAnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;
	
	MidiSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;
	double Stop_Bits;
	double Parity_Bit;
	
	//Serial analysis vars:
	U32 mSampleRateHz;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //MIDI_ANALYZER_H
