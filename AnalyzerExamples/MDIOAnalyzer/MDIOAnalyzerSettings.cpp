#include "MDIOAnalyzerSettings.h"
#include <AnalyzerHelpers.h>

MDIOAnalyzerSettings::MDIOAnalyzerSettings()
:	mMdioChannel( UNDEFINED_CHANNEL ),
	mMdcChannel( UNDEFINED_CHANNEL )
{
	mMdioChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mMdioChannelInterface->SetTitleAndTooltip( "MDIO", "MDIO data bus" );
	mMdioChannelInterface->SetChannel( mMdioChannel );

	mMdcChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mMdcChannelInterface->SetTitleAndTooltip( "MDC", "MDIO clock line " );
	mMdcChannelInterface->SetChannel( mMdcChannel );

	AddInterface( mMdioChannelInterface.get() );
	AddInterface( mMdcChannelInterface.get() );
	
	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mMdioChannel, "MDIO", false );
	AddChannel( mMdcChannel, "MDC", false );
}

MDIOAnalyzerSettings::~MDIOAnalyzerSettings()
{
}

bool MDIOAnalyzerSettings::SetSettingsFromInterfaces()
{
	if( mMdioChannelInterface->GetChannel() == mMdcChannelInterface->GetChannel() )
	{
		SetErrorText( "MDIO and MDC can't be assigned to the same input." );
		return false;
	}

	mMdioChannel = mMdioChannelInterface->GetChannel();
	mMdcChannel = mMdcChannelInterface->GetChannel();

	ClearChannels();
	AddChannel( mMdioChannel, "MDIO", true );
	AddChannel( mMdcChannel, "MDC", true );

	return true;
}

void MDIOAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mMdioChannelInterface->SetChannel( mMdioChannel );
	mMdcChannelInterface->SetChannel( mMdcChannel );
}

void MDIOAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mMdioChannel;
	text_archive >> mMdcChannel;

	ClearChannels();
	AddChannel( mMdioChannel, "MDIO", true );
	AddChannel( mMdcChannel, "MDC", true );

	UpdateInterfacesFromSettings();
}

const char* MDIOAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mMdioChannel;
	text_archive << mMdcChannel;

	return SetReturnString( text_archive.GetString() );
}
