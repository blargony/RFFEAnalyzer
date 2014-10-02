#include "UnioAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>
#include <cstring>

UnioAnalyzerSettings::UnioAnalyzerSettings()
:	mScioChannel ( UNDEFINED_CHANNEL )
{
	mScioChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mScioChannelInterface->SetTitleAndTooltip( "SCIO", "SCIO" );
	mScioChannelInterface->SetChannel( mScioChannel );

	AddInterface( mScioChannelInterface.get() );

	//AddExportOption( 0, "Export as text/csv file", "text (*.txt);;csv (*.csv)" );
	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mScioChannel, "SCIO", false );
}

UnioAnalyzerSettings::~UnioAnalyzerSettings()
{
}

void UnioAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mScioChannelInterface->SetChannel( mScioChannel );
}

bool UnioAnalyzerSettings::SetSettingsFromInterfaces()
{
	mScioChannel = mScioChannelInterface->GetChannel();

	ClearChannels();
	AddChannel( mScioChannel, "SCIO", true );

	return true;
}

void UnioAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	const char* name_string;	//the first thing in the archive is the name of the protocol analyzer that the data belongs to.
	text_archive >> &name_string;
	if( strcmp( name_string, "SaleaeUnioAnalyzer" ) != 0 )
		AnalyzerHelpers::Assert( "SaleaeUnioAnalyzer: Provided with a settings string that doesn't belong to us;" );

	text_archive >>  mScioChannel;

	ClearChannels();
	AddChannel( mScioChannel, "SCIO", true );

	UpdateInterfacesFromSettings();
}

const char* UnioAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << "SaleaeUnioAnalyzer";
	text_archive <<  mScioChannel;

	return SetReturnString( text_archive.GetString() );
}
