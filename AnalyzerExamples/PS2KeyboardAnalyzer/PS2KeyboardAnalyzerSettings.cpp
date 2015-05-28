#include "PS2KeyboardAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


PS2KeyboardAnalyzerSettings::PS2KeyboardAnalyzerSettings()
:	mClockChannel( UNDEFINED_CHANNEL ),
	mDataChannel( UNDEFINED_CHANNEL ),
	mDeviceType( 0 )
{
	mClockChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mClockChannelInterface->SetTitleAndTooltip( "Clock", "PS/2 - Clock" );
	mClockChannelInterface->SetChannel( mClockChannel );

	mDataChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mDataChannelInterface->SetTitleAndTooltip( "Data", "PS/2 - Data" );
	mDataChannelInterface->SetChannel( mDataChannel );

	mDeviceTypeInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mDeviceTypeInterface->SetTitleAndTooltip( "Device Type", "Device Type");
	mDeviceTypeInterface->AddNumber(0, "Keyboard", "Keyboard");
	mDeviceTypeInterface->AddNumber(1, "Mouse (Standard PS/2)", "Mouse (Standard PS/2)");
	mDeviceTypeInterface->AddNumber(2, "Mouse (IntelliMouse)", "Mouse (IntelliMouse)");
	mDeviceTypeInterface->SetNumber( mDeviceType );

	AddInterface( mClockChannelInterface.get() );
	AddInterface( mDataChannelInterface.get() );
	AddInterface( mDeviceTypeInterface.get() );

	AddExportOption( 0, "Export captured keys as text file (Keyboard Only)" );
	AddExportExtension( 0, "text", "txt" );
	AddExportOption( 1, "Export data as .csv log file" );
	AddExportExtension( 1, "csv", "csv" );

	ClearChannels();
	AddChannel( mClockChannel, "PS/2 - Clock", false );
	AddChannel( mDataChannel, "PS/2 - Data", false );
}

PS2KeyboardAnalyzerSettings::~PS2KeyboardAnalyzerSettings()
{
}

bool PS2KeyboardAnalyzerSettings::SetSettingsFromInterfaces()
{
	mClockChannel = mClockChannelInterface->GetChannel();
	mDataChannel = mDataChannelInterface->GetChannel();
	mDeviceType = mDeviceTypeInterface->GetNumber();
	ClearChannels();

	Channel ArrayOfChannels [2];
	ArrayOfChannels[0] = mClockChannel;
	ArrayOfChannels[1] = mDataChannel;

	bool IsInvalidConfig = AnalyzerHelpers::DoChannelsOverlap(ArrayOfChannels,2);

	if(IsInvalidConfig)
	{
		SetErrorText( "Clock and Data must be unique channels!" );
		return false;
	}
	else
	{
		AddChannel( mClockChannel, "PS/2 - Clock", true );
		AddChannel( mDataChannel, "PS/2 - Data", true );
		return true;
	}

	return true;
}

void PS2KeyboardAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mClockChannelInterface->SetChannel( mClockChannel );
	mDataChannelInterface->SetChannel( mDataChannel );
	mDeviceTypeInterface->SetNumber (mDeviceType);
}

void PS2KeyboardAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mClockChannel;
	text_archive >> mDataChannel;
	text_archive >> mDeviceType;

	ClearChannels();
	AddChannel( mClockChannel, "PS/2 - Clock", true );
	AddChannel( mDataChannel, "PS/2 - Data", true );

	UpdateInterfacesFromSettings();
}

const char* PS2KeyboardAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mClockChannel;
	text_archive << mDataChannel;
	text_archive << mDeviceType;

	return SetReturnString( text_archive.GetString() );
}
