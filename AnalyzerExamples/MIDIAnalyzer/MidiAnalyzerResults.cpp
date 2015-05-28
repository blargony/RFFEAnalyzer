#include "MidiAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "MidiAnalyzer.h"
#include "MidiAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

MidiAnalyzerResults::MidiAnalyzerResults( MidiAnalyzer* analyzer, MidiAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

MidiAnalyzerResults::~MidiAnalyzerResults()
{
}

void MidiAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );
	
	enum MidiFrameType FrameType;
	union stash stash1;
	FrameType = (enum MidiFrameType) frame.mType;
	stash1.mData2 = frame.mData2;
	
	localDisplay_base = display_base;
	localFrame = frame;
	localFrame_index = frame_index;
	
	
	results( FrameType, stash1 );
}

void MidiAnalyzerResults::results( enum MidiFrameType mFrameType, union stash mStash1 )
{	
	char * result = NULL;
	char * frameType = NULL;
	//char * channelNum = NULL;
	char channelNum[1024];
	char buffer[1024];
	const int size = 1024;
	int charsPrinted;
	int nchars;
	S8 channel = mStash1.channel;
	
	char number_str[128];
	AnalyzerHelpers::GetNumberString( localFrame.mData1, localDisplay_base, 8, number_str, 128 );

	// Makes a string describing the message type (data, command, what command).
	switch (mFrameType) {
	case Command:
		nchars = snprintf(buffer, size, "Command Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "Com Ch %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );

		AddResultString( "C" );
		break;
	case Data:
		nchars = snprintf(buffer, size, "Data Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "Data Ch %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );

		AddResultString( "D" );
		break;
	// End misc
	// Start data
	case Key:
		nchars = snprintf(buffer, size, "Key Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "Key" );
		AddResultString( "K" );
		break;
	case Velocity:
		nchars = snprintf(buffer, size, "Velocity Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );

		AddResultString( "Velocity" );
		AddResultString( "V" );
		break;
	case Touch:
		nchars = snprintf(buffer, size, "Touch Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "Touch" );
		AddResultString( "T" );
		break;
	case ControllerNum:
		nchars = snprintf(buffer, size, "ControllerNum Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "ControllerNum" );
		AddResultString( "CN" );
		break;
	case ControllerValue:
		nchars = snprintf(buffer, size, "ControllerValue Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "ControllerValue" );
		AddResultString( "CV" );
		break;
	case InstrumentNum:
		nchars = snprintf(buffer, size, "InstrumentNum Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "InstrumentNum" );
		AddResultString( "IN" );
		break;
	case Pressure:
		nchars = snprintf(buffer, size, "Pressure Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "Pressure" );
		AddResultString( "P" );
		break;
	case LSB:
		nchars = snprintf(buffer, size, "LSB Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "LSB" );
		AddResultString( "L" );
		break;
	case MSB:
		nchars = snprintf(buffer, size, "MSB Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "MSB" );
		AddResultString( "M" );
		break;
	case orphanedData:
		nchars = snprintf(buffer, size, "orphanedData Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "orphanedData" );
		AddResultString( "o" );
		break;
	// End data
	// Start commands
	case NoteOff:
		nchars = snprintf(buffer, size, "NoteOff Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "NOff Chan %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "NOff" );
		AddResultString( "N" );
		break;
	case NoteOn:
		nchars = snprintf(buffer, size, "NoteOn Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "NOn Chan %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "NOn" );
		AddResultString( "N" );
		break;
	case Aftertouch:
		nchars = snprintf(buffer, size, "Aftertouch Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "Aft Chan %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "Aft" );
		AddResultString( "A" );
		break;
	case ContinuousController:
		nchars = snprintf(buffer, size, "ContinuousController Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "ContinuControl Chan %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "ContinuControl" );
		AddResultString( "CC" );
		AddResultString( "C" );
		break;
	case PatchChange:
		nchars = snprintf(buffer, size, "PatchChange Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "P_Ch Chan %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "PCh" );
		AddResultString( "P" );
		break;
	case ChannelPressure:
		nchars = snprintf(buffer, size, "ChannelPressure Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "ChanPres Chan %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "ChanPres" );
		AddResultString( "C" );
		break;
	case PitchBend:
		nchars = snprintf(buffer, size, "PitchBend Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "PitchB Chan %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "PitchB" );
		AddResultString( "P" );
		break;
	// End commands
	// Start non-musical commands.  They do not have channels.
	case BeginSystemExclusiveMessage:
		nchars = snprintf(buffer, size, "BeginSystemExclusiveMessage  [%s]", number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "BegSystemExclMessage  [%s]", number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "BegSystemExclMessage" );
		AddResultString( "SysExclMes" );
		AddResultString( "SysEx" );
		AddResultString( "SE" );
		break;
	case MIDITimeCodeQuarterFrame:
		nchars = snprintf(buffer, size, "MIDITimeCodeQuarterFrame  [%s]", number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "TimeCodeQuarFr  [%s]", number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "TimeCodeQuarFr" );
		AddResultString( "QuarFr" );
		AddResultString( "QF" );
		break;
	case SongPositionPointer:
		nchars = snprintf(buffer, size, "SongPositionPointer  [%s]", number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "SongPosPoint  [%s]", number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "SongPosPoint" );
		AddResultString( "SongPos" );
		AddResultString( "SPP" );
		AddResultString( "SP" );
		break;
	case SongSelect:
		nchars = snprintf(buffer, size, "SongSelect  [%s]", number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		nchars = snprintf(buffer, size, "SongSel  [%s]", number_str);
		if (nchars >= 0 && nchars < size) {
			AddResultString( buffer );
		} else AddResultString( "IE" );
		
		AddResultString( "SongSel" );
		AddResultString( "SoSel" );
		AddResultString( "SS" );
		break;
	case F4:
		nchars = sprintf(channelNum, "F4  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "4" );
		break;
	case F5:
		nchars = sprintf(channelNum, "F5  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "5" );
		break;
	case TuneRequest:
		nchars = sprintf(channelNum, "TuneRequest  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "TuneReq" );
		AddResultString( "Tune" );
		AddResultString( "T" );
		break;
	case EndSystemExclusiveMessage:
		nchars = sprintf(channelNum, "EndSystemExclusiveMessage  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "EndSystemExcMess" );
		AddResultString( "EndSysExc" );
		AddResultString( "ESEM" );
		AddResultString( "ES" );
		break;
	case TimingClock:
		nchars = sprintf(channelNum, "TimingClock  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "TimClk" );
		AddResultString( "TiCl" );
		AddResultString( "TC" );
		break;
	case F9:
		nchars = sprintf(channelNum, "F9  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "9" );
		break;
	case Start_:
		nchars = sprintf(channelNum, "Start  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "Sta" );
		AddResultString( "S" );
		break;
	case Continue:
		nchars = sprintf(channelNum, "Continue  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "Cont" );
		AddResultString( "C" );
		break;
	case Stop_:
		nchars = sprintf(channelNum, "Stop  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "St" );
		AddResultString( "S" );
		break;
	case FD:
		nchars = sprintf(channelNum, "FD  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "D" );
		break;
	case ActiveSensing:
		nchars = sprintf(channelNum, "ActiveSensing  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "ActSens" );
		AddResultString( "AS" );
		break;
	case SystemReset:
		nchars = sprintf(channelNum, "SystemReset  [%s]", number_str);
		if (nchars >= 0) {
			AddResultString( channelNum );
		} else AddResultString( "IE" );
		AddResultString( "SysRes" );
		AddResultString( "SR" );
		break;
	// non-musical commands
	// Start errors
	case error:
		AddResultString( "Error" );
		AddResultString( "Err" );
		AddResultString( "E" );
		break;
	default:
		AddResultString( "Internal Error, no match" );
		AddResultString( "Internal Error" );
		AddResultString( "IntErr" );
		AddResultString( "IE" );
		break;
	}
	//	Command, Data, 
	//	NoteOff, NoteOn, Aftertouch, ContinuousController, PatchChange, ChannelPressure, PitchBend,
	//	BeginSystemExclusiveMessage, MIDITimeCodeQuarterFrame, SongPositionPointer, SongSelect, F4, F5, TuneRequest, EndSystemExclusiveMessage, 
	//	TimingClock, Start, Continue, Stop, FD, ActiveSensing, SystemReset
}

char * MidiAnalyzerResults::stringResults( enum MidiFrameType mFrameType, union stash mStash1 )
{	
	char * result = NULL;
	char * frameType = NULL;
	char channelNum[1024];
	int charsPrinted;
	S8 channel = mStash1.channel;
	
	const int size = 1024;
	char buffer[size];
	int nchars;
	
	char number_str[128];
	AnalyzerHelpers::GetNumberString( localFrame.mData1, localDisplay_base, 8, number_str, 128 );

	// Makes a string describing the message type (data, command, what command).
	switch (mFrameType) {
	case Command:
		frameType = strdup( "Command" );
		charsPrinted = sprintf(channelNum, " Channel: %i", channel);
		result = concatenateStrings( frameType, channelNum, charsPrinted );
		return result;
		break;
	case Data:
		result = strdup( "Data" );
		return result;
		break;
	// End misc
	// Start data
	case Key:
		nchars = snprintf(buffer, size, "Key Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			result = (char *) malloc ( nchars + 1 );
			strcpy (result, buffer);
		} else result = strdup( "Internal Error" );
		return result;
		break;
	case Velocity:
		nchars = snprintf(buffer, size, "Velocity Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			result = (char *) malloc ( nchars + 1 );
			strcpy (result, buffer);
		} else result = strdup( "Internal Error" );
		return result;
		break;
	case Touch:
		nchars = snprintf(buffer, size, "Touch Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			result = (char *) malloc ( nchars + 1 );
			strcpy (result, buffer);
		} else result = strdup( "Internal Error" );
		return result;
		break;
	case ControllerNum:
		nchars = snprintf(buffer, size, "ControllerNum Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			result = (char *) malloc ( nchars + 1 );
			strcpy (result, buffer);
		} else result = strdup( "Internal Error" );
		return result;
		break;
	case ControllerValue:
		nchars = snprintf(buffer, size, "ControllerValue Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			result = (char *) malloc ( nchars + 1 );
			strcpy (result, buffer);
		} else result = strdup( "Internal Error" );
		return result;
		break;
	case InstrumentNum:
		nchars = snprintf(buffer, size, "InstrumentNum Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			result = (char *) malloc ( nchars + 1 );
			strcpy (result, buffer);
		} else result = strdup( "Internal Error" );
		return result;
		break;
	case Pressure:
		nchars = snprintf(buffer, size, "Pressure Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			result = (char *) malloc ( nchars + 1 );
			strcpy (result, buffer);
		} else result = strdup( "Internal Error" );
		return result;
		break;
	case LSB:
		nchars = snprintf(buffer, size, "LSB Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			result = (char *) malloc ( nchars + 1 );
			strcpy (result, buffer);
		} else result = strdup( "Internal Error" );
		return result;
		break;
	case MSB:
		nchars = snprintf(buffer, size, "MSB Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			result = (char *) malloc ( nchars + 1 );
			strcpy (result, buffer);
		} else result = strdup( "Internal Error" );
		return result;
		break;
	case orphanedData:
		nchars = snprintf(buffer, size, "orphanedData Channel: %i [%s]", channel, number_str);
		if (nchars >= 0 && nchars < size) {
			result = (char *) malloc ( nchars + 1 );
			strcpy (result, buffer);
		} else result = strdup( "Internal Error" );
		return result;
		break;

	
	// End data
	// Start commands
	case NoteOff:
		frameType = strdup( "NoteOff" );
		charsPrinted = sprintf(channelNum, " Channel: %i", channel);
		result = concatenateStrings( frameType, channelNum, charsPrinted );
		return result;
		break;
	case NoteOn:
		frameType = strdup( "NoteOn" );
		charsPrinted = sprintf(channelNum, " Channel: %i", channel);
		result = concatenateStrings( frameType, channelNum, charsPrinted );
		return result;
		break;
	case Aftertouch:
		frameType = strdup( "Aftertouch" );
		charsPrinted = sprintf(channelNum, " Channel: %i", channel);
		result = concatenateStrings( frameType, channelNum, charsPrinted );
		return result;
		break;
	case ContinuousController:
		frameType = strdup( "ContinuousController" );
		charsPrinted = sprintf(channelNum, " Channel: %i", channel);
		result = concatenateStrings( frameType, channelNum, charsPrinted );
		return result;
		break;
	case PatchChange:
		frameType = strdup( "PatchChange" );
		charsPrinted = sprintf(channelNum, " Channel: %i", channel);
		result = concatenateStrings( frameType, channelNum, charsPrinted );
		return result;
		break;
	case ChannelPressure:
		frameType = strdup( "ChannelPressure" );
		charsPrinted = sprintf(channelNum, " Channel: %i", channel);
		result = concatenateStrings( frameType, channelNum, charsPrinted );
		return result;
		break;
	case PitchBend:
		frameType = strdup( "PitchBend" );
		charsPrinted = sprintf(channelNum, " Channel: %i", channel);
		result = concatenateStrings( frameType, channelNum, charsPrinted );
		return result;
		break;
	// End commands
	// Start non-musical commands.  They do not have channels.
	case BeginSystemExclusiveMessage:
		result = strdup( "BeginSystemExclusiveMessage" );
		return result;
		break;
	case MIDITimeCodeQuarterFrame:
		result = strdup( "MIDITimeCodeQuarterFrame" );
		return result;
		break;
	case SongPositionPointer:
		result = strdup( "SongPositionPointer" );
		return result;
		break;
	case SongSelect:
		result = strdup( "SongSelect" );
		return result;
		break;
	case F4:
		result = strdup( "F4" );
		return result;
		break;
	case F5:
		result = strdup( "F5" );
		return result;
		break;
	case TuneRequest:
		result = strdup( "TuneRequest" );
		return result;
		break;
	case EndSystemExclusiveMessage:
		result = strdup( "EndSystemExclusiveMessage" );
		return result;
		break;
	case TimingClock:
		result = strdup( "TimingClock" );
		return result;
		break;
	case F9:
		result = strdup( "F9" );
		return result;
		break;
	case Start_:
		result = strdup( "Start" );
		return result;
		break;
	case Continue:
		result = strdup( "Continue" );
		return result;
		break;
	case Stop_:
		result = strdup( "Stop" );
		return result;
		break;
	case FD:
		result = strdup( "FD" );
		return result;
		break;
	case ActiveSensing:
		result = strdup( "ActiveSensing" );
		return result;
		break;
	case SystemReset:
		result = strdup( "SystemReset" );
		return result;
		break;
	// non-musical commands
	// Start errors
	case error:
		result = strdup( "Error" );
		return result;
		break;
	default:
		result = strdup( "Internal Error, no match" );
		return result;
		break;
	}
}

void MidiAnalyzerResults::concatenateAndAddResultString( char * frameType, char * channelNum, int channelNumCharsPrinted )
{	
	if (channelNumCharsPrinted >= 0 && frameType != NULL) {
		int lengthOfFrameType = strlen(frameType);
		char * ready = (char *) alloca (lengthOfFrameType + channelNumCharsPrinted + 1 );
		strcpy (ready, frameType);
		strcat (ready, channelNum);	// Last half of result string.
		AddResultString( ready );
	} else AddResultString( "IE" );
	free( frameType );	//free( channelNum );
}

char * MidiAnalyzerResults::concatenateStrings( char * frameType, char * channelNum, int channelNumCharsPrinted )
{	
	char * result = NULL;
	if (channelNumCharsPrinted >= 0 && frameType != NULL) {
		int lengthOfFrameType = strlen(frameType);
		result = (char *) malloc (lengthOfFrameType + channelNumCharsPrinted + 1 );
		strcpy (result, frameType);
		strcat (result, channelNum);	// Last half of result string.
	} else result = strdup( "Internal Error" );
	free( frameType );	//free( channelNum );
	return result;
}

void MidiAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;
	
	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );
		
		enum MidiFrameType FrameType;
		union stash stash1;
		FrameType = (enum MidiFrameType) frame.mType;
		stash1.mData2 = frame.mData2;
		
		char * result = NULL;
		result = stringResults( FrameType, stash1 );
		
		
		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		//file_stream << time_str << "," << number_str << std::endl;
		file_stream << time_str << "," << result << "," << number_str << std::endl;
		
		free ( result );


		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void MidiAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );
    ClearTabularText();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
    AddTabularText( number_str );
}

void MidiAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void MidiAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
