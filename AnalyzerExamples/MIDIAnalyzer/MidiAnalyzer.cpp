#include "MidiAnalyzer.h"
#include "MidiAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <string.h>
/*	union stash {
		U64 mData2;
		S8 channel;
	} stash1;
*/
MidiAnalyzer::MidiAnalyzer() :	Analyzer2(),  mSettings( new MidiAnalyzerSettings() ), mSimulationInitilized( false ) {
	SetAnalyzerSettings( mSettings.get() );
	distanceFromLastCommandPacket = -1;
}

MidiAnalyzer::~MidiAnalyzer() {
	KillThread();
}

void MidiAnalyzer::WorkerThread() {

	mSampleRateHz = GetSampleRate();
	
	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );
	
	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();
	
	//double Stop_Bits = mSettings->mStopBits;
	unsigned short DataBits = 8;  // FIXME
	//bool BigEndian = 1;  // FIXME
	/*
	 * Big endian.
	 * 31.25 kBaud, 1 start, 8 data, 1 stop bit.
	 * Asynchronous
	 */
	
	U32 samples_per_bit = mSampleRateHz / mSettings->mBitRate;
	U32 samples_to_first_center_of_first_data_bit = U32( 1.5 * double( mSampleRateHz ) / double( mSettings->mBitRate ) );
	//U8 dataQueue[5];				// Storage for previous frames, arranged in a FIFO.
	//memset( &dataQueue, 0, sizeof(dataQueue) );	// clear it.
	
	for( ; ; ) {
		U8 data = 0;
		U8 mask = 1 << 7;
		enum MidiFrameType FrameType;
		union stash stash1;
		
		mSerial->AdvanceToNextEdge(); //falling edge -- beginning of the start bit
		U64 starting_sample = mSerial->GetSampleNumber();
		mSerial->Advance( samples_to_first_center_of_first_data_bit );
		
		for( U32 i=0; i < DataBits; i++ ) {	// Read in data.
			//let's put a dot exactly where we sample this bit:
			mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
			if( mSerial->GetBitState() == BIT_HIGH ) {
				data |= mask;
			}
			mSerial->Advance( samples_per_bit );
			mask = mask >> 1;
		}
		
		// Set defaults, debugging aid.
		FrameType = error;
		stash1.channel = -8;
		
		// Determine if data is a DATA byte or a COMMAND byte.
		if ( data >= 0x80 ) {			// COMMAND byte.  Data has the MSB clear, Command bytes have it set.
			if ( (data & 0xF0) == 0xF0 ) {	// All system messages have 0xF in the high nibble.
				systemMessage( data, &FrameType, &stash1 );
			} else {			
				channelMessage( data, &FrameType, &stash1 );
				distanceFromLastCommandPacket = 0;
			}
		} else {				// DATA byte
			dataParameter( data, &FrameType, &stash1 );
		}
		
		//we have a byte to save.
		Frame frame;
		frame.mData1 = data;
		frame.mData2 = stash1.mData2;
		frame.mType = FrameType;
		frame.mFlags = 0;
		frame.mStartingSampleInclusive = starting_sample;
		frame.mEndingSampleInclusive = mSerial->GetSampleNumber();
		
		mResults->AddFrame( frame );
		mResults->CommitResults();
		ReportProgress( frame.mEndingSampleInclusive );
		
		if( distanceFromLastCommandPacket != -1 ) {
			distanceFromLastCommandPacket++;
		}
	}
}

void MidiAnalyzer::dataParameter( U8 mData, enum MidiFrameType *mFrameType, union stash *mStash1)
{
	(*mStash1).channel = -2;	// Channel is not stored in a data byte.
	*mFrameType = Data;
	if( distanceFromLastCommandPacket == 1 ) {		// First parameter
		(*mStash1).channel = dataFrameCorrespondingChannel;
		switch (lastCommandPacketType) {
			case NoteOff:
				*mFrameType = Key;
				break;
			case NoteOn:
				*mFrameType = Key;
				break;
			case Aftertouch:
				*mFrameType = Key;
				break;
			case ContinuousController:
				*mFrameType = ControllerNum;
				break;
			case PatchChange:
				*mFrameType = InstrumentNum;
				break;
			case ChannelPressure:
				*mFrameType = Pressure;
				break;
			case PitchBend:
				*mFrameType = LSB;
				break;
			default:
				*mFrameType = error;
				break;
		}
	} else if( distanceFromLastCommandPacket == 2 ) {	// Second parameter
		(*mStash1).channel = dataFrameCorrespondingChannel;
		switch (lastCommandPacketType) {
			case NoteOff:
				*mFrameType = Velocity;
				break;
			case NoteOn:
				*mFrameType = Velocity;
				break;
			case Aftertouch:
				*mFrameType = Touch;
				break;
			case ContinuousController:
				*mFrameType = ControllerValue;
				break;
			case PatchChange:
				*mFrameType = error;
				break;
			case ChannelPressure:
				*mFrameType = error;
				break;
			case PitchBend:
				*mFrameType = MSB;
				break;
			default:
				*mFrameType = error;
				break;
		}
	} else {
		*mFrameType = orphanedData;
	}
}

void MidiAnalyzer::systemMessage( U8 mData, enum MidiFrameType *mFrameType, union stash *mStash1)
{
	(*mStash1).channel = -1;	// Channel is not stored in a System Message byte.
	switch (mData) {
		case 0xF0:
			*mFrameType = BeginSystemExclusiveMessage;
			break;
		case 0xF1:
			*mFrameType = MIDITimeCodeQuarterFrame;
			break;
		case 0xF2:
			*mFrameType = SongPositionPointer;
			break;
		case 0xF3:
			*mFrameType = SongSelect;
			break;
		case 0xF4:
			*mFrameType = F4;
			break;
		case 0xF5:
			*mFrameType = F5;
			break;
		case 0xF6:
			*mFrameType = TuneRequest;
			break;
		case 0xF7:
			*mFrameType = EndSystemExclusiveMessage;
			break;
		case 0xF8:
			*mFrameType = TimingClock;
			break;
		case 0xF9:
			*mFrameType = F9;
			break;
		case 0xFA:
			*mFrameType = Start_;
			break;
		case 0xFB:
			*mFrameType = Continue;
			break;
		case 0xFC:
			*mFrameType = Stop_;
			break;
		case 0xFD:
			*mFrameType = FD;
			break;
		case 0xFE:
			*mFrameType = ActiveSensing;
			break;
		case 0xFF:
			*mFrameType = SystemReset;
			break;
		default:
			*mFrameType = error;
			break;
	}
}

void MidiAnalyzer::channelMessage( U8 mData, enum MidiFrameType *mFrameType, union stash *mStash1)
{
	U8 lowerNibble = mData & 0x0F;		// Channel is stored in the lower nibble.
	(*mStash1).channel = lowerNibble;
	dataFrameCorrespondingChannel = lowerNibble;
	
	U8 mDataLowerNibbleCleared = mData & 0xF0;
	//upperNibble = upperNibble >> 4;		// Shift to lower nibble
	switch (mDataLowerNibbleCleared) {
		case 0x80:
			*mFrameType = NoteOff;
			break;
		case 0x90:
			*mFrameType = NoteOn;
			break;
		case 0xA0:
			*mFrameType = Aftertouch;
			break;
		case 0xB0:
			*mFrameType = ContinuousController;
			break;
		case 0xC0:
			*mFrameType = PatchChange;
			break;
		case 0xD0:
			*mFrameType = ChannelPressure;
			break;
		case 0xE0:
			*mFrameType = PitchBend;
			break;
		default:
			*mFrameType = error;
			(*mStash1).channel = -4;
			break;
	}
	lastCommandPacketType = *mFrameType;
}

bool MidiAnalyzer::NeedsRerun() {
    return false;
}

void MidiAnalyzer::SetupResults()
{
    mResults.reset( new MidiAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

U32 MidiAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels ) {
	if( mSimulationInitilized == false ) {
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 MidiAnalyzer::GetMinimumSampleRateHz() {
	return mSettings->mBitRate * 4;
}

const char* MidiAnalyzer::GetAnalyzerName() const {
	return "Midi";
}

const char* GetAnalyzerName() {
	return "Midi";
}

Analyzer* CreateAnalyzer() {
	return new MidiAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer ) {
	delete analyzer;
}
