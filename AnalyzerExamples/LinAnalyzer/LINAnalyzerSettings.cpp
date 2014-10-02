#include "LINAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


LINAnalyzerSettings::LINAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL )
,	mLINVersion( 2.0 )
,	mBitRate( 20000 )
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Serial", "Standard LIN" );
	mInputChannelInterface->SetChannel( mInputChannel );

	mLINVersionInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mLINVersionInterface->SetTitleAndTooltip( "LIN Version",  "Specify the LIN protocol version 1 or 2." );
	mLINVersionInterface->AddNumber( 1.0, "Version 1.x", "LIN Protocol Specification Version 1.x");
	mLINVersionInterface->AddNumber( 2.0, "Version 2.x", "LIN Protocol Specification Version 2.x");
	mLINVersionInterface->SetNumber( mLINVersion );

	mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBitRateInterface->SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface->SetMax( 20000 );
	mBitRateInterface->SetMin( 1000 );
	mBitRateInterface->SetInteger( mBitRate );

	AddInterface( mInputChannelInterface.get() );
	AddInterface( mLINVersionInterface.get() );
	AddInterface( mBitRateInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mInputChannel, "Serial", false );
}

LINAnalyzerSettings::~LINAnalyzerSettings()
{
}

bool LINAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	mLINVersion = mLINVersionInterface->GetNumber();
	mBitRate = mBitRateInterface->GetInteger();

	ClearChannels();
	AddChannel( mInputChannel, "LIN", true );

	return true;
}

void LINAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
	mLINVersionInterface->SetNumber( mLINVersion );
	mBitRateInterface->SetInteger( mBitRate );
}

void LINAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mBitRate;
	text_archive >> mLINVersion;

	ClearChannels();
	AddChannel( mInputChannel, "LIN", true );

	UpdateInterfacesFromSettings();
}

const char* LINAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mBitRate;
	text_archive << mLINVersion;

	return SetReturnString( text_archive.GetString() );
}
