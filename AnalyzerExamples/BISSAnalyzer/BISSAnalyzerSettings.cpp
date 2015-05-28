#include "BISSAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


BISSAnalyzerSettings::BISSAnalyzerSettings()
:	mMaChannel( UNDEFINED_CHANNEL ),
    mSloChannel( UNDEFINED_CHANNEL ),
	mDataLength( 9 ),
	mDatenart()
	///mBitRate( 9600 )
{

//---------------------------------------------------------------------------------------------------------------------------------
	mMaChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mMaChannelInterface->SetTitleAndTooltip( "MA:", "MA" );
	mMaChannelInterface->SetChannel( mMaChannel );
//---------------------------------------------------------------------------------------------------------------------------------
	mSloChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mSloChannelInterface->SetTitleAndTooltip( "SLO:", "SLO" );
	mSloChannelInterface->SetChannel( mSloChannel );	
//---------------------------------------------------------------------------------------------------------------------------------
	mDatenartInterface.reset( new AnalyzerSettingInterfaceNumberList() );
	mDatenartInterface->SetTitleAndTooltip("Type of data:", "Specify the type of data to be analyzed." );
	mDatenartInterface->AddNumber(0,"Register Data","");
	mDatenartInterface->AddNumber(1,"Single Cycle Data","");
	mDatenartInterface->SetNumber( mDatenart );
//---------------------------------------------------------------------------------------------------------------------------------
	mDataLengthInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mDataLengthInterface->SetTitleAndTooltip( "Serial data length (Bit):",  "Specify the serial data length in bit." );
	mDataLengthInterface->SetMax( 64 );
	mDataLengthInterface->SetMin( 9 );
	mDataLengthInterface->SetInteger( mDataLength );
//---------------------------------------------------------------------------------------------------------------------------------
	AddInterface( mMaChannelInterface.get() );	
	AddInterface( mSloChannelInterface.get() );
	AddInterface( mDatenartInterface.get() );
	AddInterface( mDataLengthInterface.get() );
	//AddInterface( mBitRateInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();
	AddChannel( mMaChannel, "MA", false );
	AddChannel( mSloChannel, "SLO", false );
//---------------------------------------------------------------------------------------------------------------------------------

	/*
	mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBitRateInterface->SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface->SetMax( 6000000 );
	mBitRateInterface->SetMin( 1 );
	mBitRateInterface->SetInteger( mBitRate );
*/
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
BISSAnalyzerSettings::~BISSAnalyzerSettings()
//---------------------------------------------------------------------------------------------------------------------------------
{
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
bool BISSAnalyzerSettings::SetSettingsFromInterfaces()
//---------------------------------------------------------------------------------------------------------------------------------
{
	mMaChannel = mMaChannelInterface->GetChannel();
	mSloChannel = mSloChannelInterface->GetChannel();
	//mBitRate = mBitRateInterface->GetInteger();
	mDataLength = mDataLengthInterface->GetInteger();
	mDatenart = mDatenartInterface->GetNumber();

	ClearChannels();
	AddChannel( mMaChannel, "MA", true );
	AddChannel( mSloChannel, "SLO", true );

	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzerSettings::UpdateInterfacesFromSettings()
//---------------------------------------------------------------------------------------------------------------------------------
{
	mMaChannelInterface->SetChannel( mMaChannel );
	mSloChannelInterface->SetChannel( mSloChannel );
	//mBitRateInterface->SetInteger( mBitRate );
	mDataLengthInterface->SetInteger( mDataLength);
	mDatenartInterface->SetNumber( mDatenart );
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzerSettings::LoadSettings( const char* settings )
//---------------------------------------------------------------------------------------------------------------------------------
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mMaChannel;
	text_archive >> mSloChannel;
	//text_archive >> mBitRate;
	text_archive >> mDataLength;
	text_archive >> mDatenart;

	ClearChannels();
	AddChannel( mMaChannel, "MA", true );
	AddChannel( mSloChannel, "SLO", true );

	UpdateInterfacesFromSettings();
}
//---------------------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------------------
const char* BISSAnalyzerSettings::SaveSettings()
//---------------------------------------------------------------------------------------------------------------------------------
{
	SimpleArchive text_archive;

	text_archive << mMaChannel;
	text_archive << mSloChannel;
	//text_archive << mBitRate;
	text_archive << mDataLength;
	text_archive << mDatenart;

	return SetReturnString( text_archive.GetString() );
}
//---------------------------------------------------------------------------------------------------------------------------------
