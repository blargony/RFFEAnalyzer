#include "MidiAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


MidiAnalyzerSettings::MidiAnalyzerSettings() :	mInputChannel( UNDEFINED_CHANNEL ), mBitRate( 31250 ) {
	mParity = 3; // default is No parity.
	mStopBits = 1;  // default is 1 stop bit.  1=1, 2=1.5, 3=2.
	mDataBits = 8;  // default
	mEndianness = 1;  // TRUE, Big Endian is default.  0=FALSE=LE, 1=TRUE=BE.
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "MIDI", "General MIDI" );
	mInputChannelInterface->SetChannel( mInputChannel );
/*
	mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBitRateInterface->SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface->SetMax( 31250 );
	mBitRateInterface->SetMin( 31250 ); // Midi only has one speed.  Tolerance is +/- 1%.
	mBitRateInterface->SetInteger( mBitRate );

	mStopBitsInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mStopBitsInterface->SetTitleAndTooltip( "Stop Bits", "Specify number of stop bits, 1, 1.5, or 2." );
	mStopBitsInterface->AddNumber( 1, "1", "1 stop bit (common)");
	mStopBitsInterface->AddNumber( 1.5, "1.5", "1.5 stop bits (rare)");
	mStopBitsInterface->AddNumber( 2, "2", "2 stop bits (less common, better for noisy lines)");
	mStopBitsInterface->SetNumber( mStopBits );

	mParityInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mParityInterface->SetTitleAndTooltip( "Parity", "Parity:  Even, Odd, or None." );
	mParityInterface->AddNumber( 1, "Even", "Even Parity");
	mParityInterface->AddNumber( 2, "Odd", "Odd Parity");
	mParityInterface->AddNumber( 3, "None", "No Parity");
	mParityInterface->SetNumber( mParity );

	// FIXME: the below is unfinished!
	mDataBitsInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mDataBitsInterface->SetTitleAndTooltip( "Data Bits", "" );
	mDataBitsInterface->AddNumber( 1, "Even", "Even Parity");
	mDataBitsInterface->AddNumber( 2, "Odd", "Odd Parity");
	mDataBitsInterface->AddNumber( 3, "None", "No Parity");
	mDataBitsInterface->SetNumber( mParity );

	mEndiannessInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mEndiannessInterface->SetTitleAndTooltip( "Endianness", "Little or Big Endian" );
	mEndiannessInterface->AddNumber( 0, "Little", "Odd Parity");
	mEndiannessInterface->AddNumber( 1, "Big", "Even Parity");
	mEndiannessInterface->SetNumber( mParity );
*/

	AddInterface( mInputChannelInterface.get() );
	//AddInterface( mBitRateInterface.get() );
	//AddInterface( mStopBitsInterface.get() );
	//AddInterface( mParityInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "MIDI", false );
}

MidiAnalyzerSettings::~MidiAnalyzerSettings()
{
}

bool MidiAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	//mBitRate = mBitRateInterface->GetInteger();
	//mStopBits = mStopBitsInterface->GetNumber();
	//mParity = mParityInterface->GetNumber();

	ClearChannels();
	AddChannel( mInputChannel, "MIDI", true );

	return true;
}

void MidiAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
	//mBitRateInterface->SetInteger( mBitRate );
	//mStopBitsInterface->SetNumber( mStopBits );
	//mParityInterface->SetNumber( mParity );
}

void MidiAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mBitRate;
	text_archive >> mStopBits;
	text_archive >> mParity;

	ClearChannels();
	AddChannel( mInputChannel, "MIDI", true );

	UpdateInterfacesFromSettings();
}

const char* MidiAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mBitRate;
	text_archive << mStopBits;
	text_archive << mParity;

	return SetReturnString( text_archive.GetString() );
}
