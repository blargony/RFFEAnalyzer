#include "CanAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>

CanAnalyzerSettings::CanAnalyzerSettings()
:	mCanChannel ( UNDEFINED_CHANNEL ),
	mBitRate ( 1000000 )
{
	mCanChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mCanChannelInterface->SetTitleAndTooltip( "CAN", "Controller Area Network - Input" );
	mCanChannelInterface->SetChannel( mCanChannel );

	mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBitRateInterface->SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface->SetMax( 2000000 );
	mBitRateInterface->SetMin( 10000 );
	mBitRateInterface->SetInteger( mBitRate );

	AddInterface( mCanChannelInterface.get() );
	AddInterface( mBitRateInterface.get() );

	//AddExportOption( 0, "Export as text/csv file", "text (*.txt);;csv (*.csv)" );
	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mCanChannel, "CAN", false );
}

CanAnalyzerSettings::~CanAnalyzerSettings()
{
}

bool CanAnalyzerSettings::SetSettingsFromInterfaces()
{
	Channel can_channel = mCanChannelInterface->GetChannel();
	
	if( can_channel == UNDEFINED_CHANNEL )
	{
		SetErrorText( "Please select a channel for the CAN interface" );
		return false;
	}
	mCanChannel = can_channel;
	mBitRate = mBitRateInterface->GetInteger();

	ClearChannels();
	AddChannel( mCanChannel, "CAN", true );

	return true;
}

void CanAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	const char* name_string;	//the first thing in the archive is the name of the protocol analyzer that the data belongs to.
	text_archive >> &name_string;
	if( strcmp( name_string, "SaleaeCANAnalyzer" ) != 0 )
		AnalyzerHelpers::Assert( "SaleaeCanAnalyzer: Provided with a settings string that doesn't belong to us;" );

	text_archive >>  mCanChannel;
	text_archive >>  mBitRate;

	ClearChannels();
	AddChannel( mCanChannel, "CAN", true );

	UpdateInterfacesFromSettings();
}

const char* CanAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive <<  "SaleaeCANAnalyzer";
	text_archive <<  mCanChannel;
	text_archive << mBitRate;

	return SetReturnString( text_archive.GetString() );
}

void CanAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mCanChannelInterface->SetChannel( mCanChannel );
	mBitRateInterface->SetInteger( mBitRate );
}
