#ifndef MIDI_ANALYZER_RESULTS
#define MIDI_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class MidiAnalyzer;
class MidiAnalyzerSettings;

//enum CanFrameType { IdentifierField, IdentifierFieldEx, ControlField, DataField, CrcField, AckField, CanError };
enum MidiFrameType {	// Note: something else had the keywords Start and Stop; appended '_' to these.
	Command, Data,												// Meta, probably not used.
	NoteOff, NoteOn, Aftertouch, ContinuousController, PatchChange, ChannelPressure, PitchBend,		// Commands
	Key, Velocity, Touch, ControllerNum, ControllerValue, InstrumentNum, Pressure, LSB, MSB, orphanedData,	// Data
	BeginSystemExclusiveMessage, MIDITimeCodeQuarterFrame, SongPositionPointer, SongSelect,			// non-musical commands
	F4, F5, TuneRequest, EndSystemExclusiveMessage,								// Ditto
	TimingClock, F9, Start_, Continue, Stop_, FD, ActiveSensing, SystemReset,				// Ditto
	error
};

class MidiAnalyzerResults : public AnalyzerResults
{
public:
	MidiAnalyzerResults( MidiAnalyzer* analyzer, MidiAnalyzerSettings* settings );
	virtual ~MidiAnalyzerResults();
	
	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );
	
	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected:	//vars
	struct node {
		char * data;
		struct node * next;
	};
	union stash {
		U64 mData2;
		S8 channel;
		S8 commandOffset;
	} stash1;
	DisplayBase localDisplay_base;	// Set by GenerateBubbleText.  Made local variable to avoid shipping.
	Frame localFrame;		// Same.
	U64 localFrame_index;		// Same.
	
protected:	//functions
	//struct node * frameType( enum MidiFrameType mFrameType );
	void results( enum MidiFrameType mFrameType, union stash mStash1 );
	void concatenateAndAddResultString( char * frameType, char * channelNum, int channelNumCharsPrinted );
	char * stringResults( enum MidiFrameType mFrameType, union stash mStash1 );
	char * concatenateStrings( char * frameType, char * channelNum, int channelNumCharsPrinted );
	/*
	#ifndef __USE_GNU
	int asprintf (char **__restrict __ptr, __const char *__restrict __fmt, ...);
	#endif
	*/

protected:	//vars
	MidiAnalyzerSettings* mSettings;
	MidiAnalyzer* mAnalyzer;
};


#endif //MIDI_ANALYZER_RESULTS
