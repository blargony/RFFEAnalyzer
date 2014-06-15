#include "Dmx512AnalyzerSettings.h"
#include <AnalyzerHelpers.h>

Dmx512AnalyzerSettings::Dmx512AnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mBitRate( 250000 ),
	mMinMAB( .000004 )
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Serial", "Standard DMX-512" );
	mInputChannelInterface->SetChannel( mInputChannel );
	AddInterface( mInputChannelInterface.get() );
	
	mOldVersionInterface.reset( new AnalyzerSettingInterfaceBool() );
	mOldVersionInterface->SetTitleAndTooltip( "Accept DMX-1986 4us MAB",
											  "Accept 4us MAB as per USITT DMX-512 (1986)" ); 
	AddInterface( mOldVersionInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "Serial", false );
}

Dmx512AnalyzerSettings::~Dmx512AnalyzerSettings()
{
}

bool Dmx512AnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();

	ClearChannels();
	AddChannel( mInputChannel, "DMX-512", true );

	mMinMAB = mOldVersionInterface->GetValue() ? .000004 : .000008;
	
	return true;
}

void Dmx512AnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
	mOldVersionInterface->SetValue( mMinMAB < .000008 );
}

void Dmx512AnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mMinMAB;

	ClearChannels();
	AddChannel( mInputChannel, "DMX-512", true );

	UpdateInterfacesFromSettings();
}

const char* Dmx512AnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;
	
	text_archive << mInputChannel;
	text_archive << mMinMAB;

	return SetReturnString( text_archive.GetString() );
}
