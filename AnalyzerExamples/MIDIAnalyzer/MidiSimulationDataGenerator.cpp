#include "MidiSimulationDataGenerator.h"
#include "MidiAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <string.h>
#include <stdlib.h>

MidiSimulationDataGenerator::MidiSimulationDataGenerator()
{
	//loadTestData();
	loadSampleData();
}

void MidiSimulationDataGenerator::loadTestData()
{		// Only for testing
	mMidiBuf[0] = 0x81;
	mMidiBuf[1] = 0x92;
	mMidiBuf[2] = 0xa0;
	mMidiBuf[3] = 0xb1;
	mMidiBuf[4] = 0xc2;
	mMidiBuf[5] = 0xd4;
	mMidiBuf[6] = 0xe5;
	
	mMidiBuf[7] = 0xf0;
	mMidiBuf[8] = 0xf1;
	mMidiBuf[9] = 0xf2;
	mMidiBuf[10] = 0xf3;
	mMidiBuf[11] = 0xf4;
	mMidiBuf[12] = 0xf5;
	mMidiBuf[13] = 0xf6;
	mMidiBuf[14] = 0xff;
	mMidiBuf[15] = 0xf8;
	mMidiBuf[16] = 0xf9;
	mMidiBuf[17] = 0xfa;
	mMidiBuf[18] = 0xfb;
	mMidiBuf[19] = 0xfc;
	mMidiBuf[20] = 0xfd;
	mMidiBuf[21] = 0xfe;
	mMidiBuf[22] = 0xf7;
	
	mMidiBuf[23] = 0xb1;
	mMidiBuf[24] = 0xc2;

	mMidiBufIndex = 0;
	mMidiBufShortEnd = 25;	// Not relevant here, but needs to be initialized to a 
				// value >= the size of the storage array.
}

void MidiSimulationDataGenerator::loadSampleData()
{		// Plausible MIDI for end-user
	mMidiBuf[0] = 0x90;
	mMidiBuf[1] = 0x3f;
	mMidiBuf[2] = 0x40;
	mMidiBuf[3] = 0x81;
	mMidiBuf[4] = 0x3f;
	mMidiBuf[5] = 0x40;
	mMidiBuf[6] = 0x92;
	mMidiBuf[7] = 0x3c;
	mMidiBuf[8] = 0x40;
	mMidiBuf[9] = 0x83;
	mMidiBuf[10] = 0x3c;
	mMidiBuf[11] = 0x40;
	
	mMidiBufIndex = 0;
	mMidiBufShortEnd = 12;	// Needed to prevent reading past the initialized part of the array.
	/*
	 * 672: 90 3f 40 note on (channel 0): pitch 63, velocity 64
	480: 80 3f 40 note off (channel 0): pitch 63, velocity 64
	384: 90 3c 40 note on (channel 0): pitch 60, velocity 64
	192: 80 3c 40 note off (channel 0): pitch 60, velocity 64
	 */
}

void MidiSimulationDataGenerator::injectBurstErrors()
{
	// This function adds random transitions while advancing the stream.
	// Pseudocode Example:
	// write numSamplesToOverwrite samples of garbage.
	// done.
	
	const int startingSample = mMidiSimulationData.GetCurrentSampleNumber();
	U32 samples_per_bit = mSimulationSampleRateHz / mSettings->mBitRate;
	const int numSamplesToOverwrite = 13 * samples_per_bit;
	U64 currentSample = startingSample;
	int r;
	float probability = 0.2f;
	srand( 1 );		// Default seed, will produce repeatable sequences.
	while ( currentSample < ( startingSample + numSamplesToOverwrite ) ) {
		mMidiSimulationData.Advance( 1 );
		r = rand();
		if ( (float) r < ( (float)RAND_MAX * probability ) ) {
			mMidiSimulationData.Transition();  //
		}
		currentSample++;
	}
}

void MidiSimulationDataGenerator::CreateMidiMessage()
{
	U32 samples_per_bit = mSimulationSampleRateHz / mSettings->mBitRate;
	
	U8 byte = mMidiBuf[ mMidiBufIndex ];
	
	//we're currenty high
	//let's move forward a little
	mMidiSimulationData.Advance( samples_per_bit * 3 );
	
	mMidiSimulationData.Transition();  //low-going edge for start bit
	mMidiSimulationData.Advance( samples_per_bit );  //add start bit time
	
	U8 mask = 1 << 7;
	U32 dataBitsCounter = 0;
	for( dataBitsCounter = 0; dataBitsCounter < 8; dataBitsCounter++ )
	{
		if( ( byte & mask ) != 0 )
			mMidiSimulationData.TransitionIfNeeded( BIT_HIGH );
		else
			mMidiSimulationData.TransitionIfNeeded( BIT_LOW );
		
		mMidiSimulationData.Advance( samples_per_bit );
		
		mask = mask >> 1;
	}
	
	mMidiSimulationData.TransitionIfNeeded( BIT_HIGH ); //we need to end high
	
	//lets pad the end a bit for the stop bit:
	mMidiSimulationData.Advance( samples_per_bit );
	if( mMidiBufIndex == 15 && mMidiBufShortEnd == 25 && false ) {
		injectBurstErrors();
		mMidiBufIndex++;
	}
	
	mMidiBufIndex++;
	if( mMidiBufIndex >= (sizeof(mMidiBuf) / sizeof(U32)) || mMidiBufIndex >= mMidiBufShortEnd ) {
		mMidiBufIndex = 0;
		mMidiSimulationData.Advance( samples_per_bit * 30 );
	}
}

MidiSimulationDataGenerator::~MidiSimulationDataGenerator()
{
	
}

void MidiSimulationDataGenerator::Initialize( U32 simulation_sample_rate, MidiAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;
	
	mMidiSimulationData.SetChannel( mSettings->mInputChannel );
	mMidiSimulationData.SetSampleRate( simulation_sample_rate );
	mMidiSimulationData.SetInitialBitState( BIT_HIGH );
}

U32 MidiSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );
	
	while( mMidiSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		CreateMidiMessage();
	}
	
	*simulation_channel = &mMidiSimulationData;
	return 1;
}

/*
 * Big endian.
 * 31.25 kBaud, 1 start, 8 data, 1 stop bit.
 * Asynchronous
 * 
 * Midi is transmitted as asynchronous
bytes at 31250 bits per second.
One start bit, eight data bits,
and one stop bit means a maximum
transmission rate of 3125 bytes
per second. If the first bit is
set, the byte is a Status Byte.
The Status Byte determines the
length of most messages, which are
usually one, two, or three bytes
in length. System Exclusive messages
are variable length, and have a
beginning and ending status byte.
 * 
 * 
 * Source: http://soundlab.cs.princeton.edu/learning/tutorials/InputMidi/midisoft.html
 */

/*
 * MidiAnalyzer $ jack_midi_dump dumpMidi
672: 90 3f 40 note on (channel 0): pitch 63, velocity 64
480: 80 3f 40 note off (channel 0): pitch 63, velocity 64
384: 90 3c 40 note on (channel 0): pitch 60, velocity 64
192: 80 3c 40 note off (channel 0): pitch 60, velocity 64
96: 90 3f 40 note on (channel 0): pitch 63, velocity 64
928: 80 3f 40 note off (channel 0): pitch 63, velocity 64
832: 90 3c 40 note on (channel 0): pitch 60, velocity 64
640: 80 3c 40 note off (channel 0): pitch 60, velocity 64
544: 90 3f 40 note on (channel 0): pitch 63, velocity 64
352: 80 3f 40 note off (channel 0): pitch 63, velocity 64
^C
 */

/*
 Organization Midi commands and data in a byte of information

    Midi bytes range between 0 and 255, or shown below in various representations:

    Numeric range of Midi bytes

        decimal     hexadecimal          binary
    =======================================================
           0               0                 0
         255              FF          11111111

    Midi commands and data are distinguished according to the most significant bit of the byte. 
    * If there is a zero in the top bit, then the byte is a data byte, and if there is a one in the top bit, then the byte is a command byte. 
    * Here is how they are separated:

    Division of data and commands by values

        decimal     hexadecimal          binary
    =======================================================
    DATA bytes:
           0               0          00000000
         ...             ...               ...
         127              7F          01111111

    COMMAND bytes:
         128              80          10000000
         ...             ...               ...
         255              FF          11111111

    Furthermore, command bytes are split into half. 
    * The most significant half contains the actual Midi command, and the second half contains the Midi channel 
    * for which the command is for. For example, 0x91 is the note-on command for the second Midi channel. 
    * The 9 digit is the actual command for note-on and the digit 1 specifies the second channel (the first channel being 0). 
    * The 0xF0 set of commands do not follow this convention.

    Here is a table of the Midi commands:

    Midi commands

       0x80     Note Off
       0x90     Note On
       0xA0     Aftertouch
       0xB0     Continuous controller
       0xC0     Patch change
       0xD0     Channel Pressure
       0xE0     Pitch bend
       0xF0     (non-musical commands)

    The messages from 0x80 to 0xEF are called Channel Messages because the second four bits 
    * of the command specify which channel the message affects. 
    * The messages from 0xF0 to 0xFF are called System Messages; they do not affect any particular channel. 

Midi messages

    A Midi command plus its Midi data parameters to be called a Midi message. 
    * The minimum size of a Midi message is 1 byte (one command byte and no parameter bytes). 
    * The maximum size of a Midi message (note considering 0xF0 commands) is three bytes. 
    * A Midi message always starts with a command byte. 
    * Here is a table of the Midi messages that are possible in the Midi protocol:

    Command 	Meaning 		# parameters 	param 1 	param 2
    0x80 	Note-off 		2 		key 		velocity
    0x90 	Note-on 		2 		key 		velocity
    0xA0 	Aftertouch 		2 		key 		touch
    0xB0 	Continuous controller 	2 		controller # 	controller value
    0xC0 	Patch change 		2 		instrument # 	
    0xD0 	Channel Pressure 	1 		pressure
    0xE0 	Pitch bend 		2 		lsb (7 bits) 	msb (7 bits)
    0xF0 	(non-musical commands) 			

    I won't discuss the 0xF0 set of commands (System messages) here very much, but here is a basic table of them:

    command 	meaning 	# param
    0xF0 	start of system exclusive message 	variable
    0xF1 	Midi Time Code Quarter Frame (Sys Common)	
    0xF2 	Song Position Pointer (Sys Common)	
    0xF3 	Song Select (Sys Common) 	
    0xF4 	??? 	
    0xF5 	??? 	
    0xF6 	Tune Request (Sys Common) 	
    0xF7 	end of system exclusive message 	0
    0xF8 	Timing Clock (Sys Realtime) 	
    0xFA 	Start (Sys Realtime) 	
    0xFB 	Continue (Sys Realtime) 	
    0xFC 	Stop (Sys Realtime) 	
    0xFD 	??? 	
    0xFE 	Active Sensing (Sys Realtime) 	
    0xFF 	System Reset (Sys Realtime) 	

    Running status should be mentioned around here... 
    * 
    * Source: https://ccrma.stanford.edu/~craig/articles/linuxmidi/misc/essenmidi.html
*/
