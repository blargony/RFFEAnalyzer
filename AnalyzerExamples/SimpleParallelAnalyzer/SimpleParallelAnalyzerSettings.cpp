#include "SimpleParallelAnalyzerSettings.h"
#include <AnalyzerHelpers.h>
#include <stdio.h>


#pragma warning( disable : 4996 ) //warning C4996: 'sprintf': This function or variable may be unsafe

SimpleParallelAnalyzerSettings::SimpleParallelAnalyzerSettings()
:
	mClockChannel( UNDEFINED_CHANNEL ),
	mClockEdge( AnalyzerEnums::PosEdge )
{
	U32 count = 16;
	for( U32 i=0; i<count; i++ )
	{
		mDataChannels.push_back( UNDEFINED_CHANNEL );
		AnalyzerSettingInterfaceChannel* data_channel_interface = new AnalyzerSettingInterfaceChannel();
		
		char text[64];
		sprintf( text, "D%d", i );

		data_channel_interface->SetTitleAndTooltip( text, text );
		data_channel_interface->SetChannel( mDataChannels[i] );
		data_channel_interface->SetSelectionOfNoneIsAllowed( true );

		mDataChannelsInterface.push_back( data_channel_interface );
	}


	mClockChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mClockChannelInterface->SetTitleAndTooltip( "Clock", "Clock" );
	mClockChannelInterface->SetChannel( mClockChannel );

	mClockEdgeInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mClockEdgeInterface->SetTitleAndTooltip( "", "" );
	mClockEdgeInterface->AddNumber( AnalyzerEnums::PosEdge, "Data is valid on Clock rising edge", "Data is valid on Clock rising edge" );
	mClockEdgeInterface->AddNumber( AnalyzerEnums::NegEdge, "Data is valid on Clock falling edge", "Data is valid on Clock falling edge" );
	mClockEdgeInterface->SetNumber( mClockEdge );



	for( U32 i=0; i<count; i++ )
	{
		AddInterface( mDataChannelsInterface[i] );
	}

	AddInterface( mClockChannelInterface.get() );
	AddInterface( mClockEdgeInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	for( U32 i=0; i<count; i++ )
	{
		char text[64];
		sprintf( text, "D%d", i );
		AddChannel( mDataChannels[i], text, false );
	}

	AddChannel( mClockChannel, "Clock", false );
}

SimpleParallelAnalyzerSettings::~SimpleParallelAnalyzerSettings()
{
	U32 count = mDataChannelsInterface.size();
	for( U32 i=0; i<count; i++ )
		delete mDataChannelsInterface[i];
}

bool SimpleParallelAnalyzerSettings::SetSettingsFromInterfaces()
{
	U32 count = mDataChannels.size();
	U32 num_used_channels = 0;
	for( U32 i=0; i<count; i++ )
	{
		if( mDataChannelsInterface[i]->GetChannel() != UNDEFINED_CHANNEL )
			num_used_channels++;
	}

	if( num_used_channels == 0 )
	{
		SetErrorText( "Please select at least one channel to use in the parallel bus" );
		return false;
	}

	for( U32 i=0; i<count; i++ )
	{
		mDataChannels[i] = mDataChannelsInterface[i]->GetChannel();
	}

	mClockChannel = mClockChannelInterface->GetChannel();
	mClockEdge = AnalyzerEnums::EdgeDirection( U32( mClockEdgeInterface->GetNumber() ) );

	ClearChannels();
	for( U32 i=0; i<count; i++ )
	{
		char text[64];
		sprintf( text, "D%d", i );
		AddChannel( mDataChannels[i], text, mDataChannels[i] != UNDEFINED_CHANNEL );
	}

	AddChannel( mClockChannel, "Clock", true );

	return true;
}

void SimpleParallelAnalyzerSettings::UpdateInterfacesFromSettings()
{
	U32 count = mDataChannels.size();
	for( U32 i=0; i<count; i++ )
	{
		mDataChannelsInterface[i]->SetChannel( mDataChannels[i] );
	}

	mClockChannelInterface->SetChannel( mClockChannel );
	mClockEdgeInterface->SetNumber( mClockEdge );
}

void SimpleParallelAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	U32 count = mDataChannels.size();

	for( U32 i=0; i<count; i++ )
	{
		text_archive >> mDataChannels[i];
	}

	text_archive >> mClockChannel;
	text_archive >> *(U32*)&mClockEdge;

	ClearChannels();
	for( U32 i=0; i<count; i++ )
	{
		char text[64];
		sprintf( text, "D%d", i );
		AddChannel( mDataChannels[i], text, mDataChannels[i] != UNDEFINED_CHANNEL );
	}

	AddChannel( mClockChannel, "Clock", true );

	UpdateInterfacesFromSettings();
}

const char* SimpleParallelAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

		U32 count = mDataChannels.size();

	for( U32 i=0; i<count; i++ )
	{
		text_archive << mDataChannels[i];
	}

	text_archive << mClockChannel;
	text_archive << mClockEdge;

	return SetReturnString( text_archive.GetString() );
}
