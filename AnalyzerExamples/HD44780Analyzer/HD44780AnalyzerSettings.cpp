#include "HD44780AnalyzerSettings.h"
#include <AnalyzerHelpers.h>
#include <stdio.h>
#include <string.h>
#define ANALYZER_SAVE_NAME "SaleaeHD44780Analyzer"
#define ANALYZER_SAVE_VERSION 1

HD44780AnalyzerSettings::HD44780AnalyzerSettings()
  : mEChannel( UNDEFINED_CHANNEL ),
    mRSChannel( UNDEFINED_CHANNEL ),
    mRWChannel( UNDEFINED_CHANNEL ),
    mMarkTimingErrors(true),
    mEnableCycleMin(500),
    mEnablePulseWidthMin(230),
    mAddressSetupMin(40),
    mAddressHoldMin(10),
    mDataWriteSetupMin(80),
    mDataWriteHoldMin(10),
    mDataReadDelayMax(160),
    mDataReadHoldMin(5),
    mIgnoreEPulsesWhenBusy(true),
    mBusyTimeClearHome(1520),
    mBusyTimeCmdChar(37),
    mStartIn4BitMode(false),
    mDoNotGenerateBusyCheckFrames(false)
{
  U32 dbline;

  //initialize mDBChannels (is there a way to init an array liek this above like the others?)
  for (dbline=0;dbline<8;dbline++)
    mDBChannel[dbline]=UNDEFINED_CHANNEL;

  //setup interfaces
	mEChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mEChannelInterface->SetTitleAndTooltip( "E Signal", "E is the enable signal." );
	mEChannelInterface->SetChannel( mEChannel );

	mRSChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mRSChannelInterface->SetTitleAndTooltip( "RS Signal", "RS is the register select signal." );
	mRSChannelInterface->SetChannel( mRSChannel );

	mRWChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mRWChannelInterface->SetTitleAndTooltip( "RW Signal", "RW is the read write signal (none=writing only, RW always grounded)." );
  mRWChannelInterface->SetSelectionOfNoneIsAllowed(true);
	mRWChannelInterface->SetChannel( mRWChannel );

	mDBChannelInterface[0].reset( new AnalyzerSettingInterfaceChannel() );
	mDBChannelInterface[0]->SetTitleAndTooltip( "DB0 Signal", "DB0 is the data bit 0 signal (none=4-bit mode)." );
  mDBChannelInterface[0]->SetSelectionOfNoneIsAllowed(true);
	mDBChannelInterface[0]->SetChannel( mDBChannel[0] );

	mDBChannelInterface[1].reset( new AnalyzerSettingInterfaceChannel() );
	mDBChannelInterface[1]->SetTitleAndTooltip( "DB1 Signal", "DB1 is the data bit 1 signal (none=4-bit mode)." );
  mDBChannelInterface[1]->SetSelectionOfNoneIsAllowed(true);
	mDBChannelInterface[1]->SetChannel( mDBChannel[1] );

	mDBChannelInterface[2].reset( new AnalyzerSettingInterfaceChannel() );
	mDBChannelInterface[2]->SetTitleAndTooltip( "DB2 Signal", "DB2 is the data bit 2 signal (none=4-bit mode)." );
  mDBChannelInterface[2]->SetSelectionOfNoneIsAllowed(true);
	mDBChannelInterface[2]->SetChannel( mDBChannel[2] );

	mDBChannelInterface[3].reset( new AnalyzerSettingInterfaceChannel() );
	mDBChannelInterface[3]->SetTitleAndTooltip( "DB3 Signal", "DB3 is the data bit 3 signal (none=4-bit mode)." );
  mDBChannelInterface[3]->SetSelectionOfNoneIsAllowed(true);
	mDBChannelInterface[3]->SetChannel( mDBChannel[3] );

	mDBChannelInterface[4].reset( new AnalyzerSettingInterfaceChannel() );
	mDBChannelInterface[4]->SetTitleAndTooltip( "DB4 Signal", "DB4 is the data bit 4." );
	mDBChannelInterface[4]->SetChannel( mDBChannel[4] );

	mDBChannelInterface[5].reset( new AnalyzerSettingInterfaceChannel() );
	mDBChannelInterface[5]->SetTitleAndTooltip( "DB5 Signal", "DB5 is the data bit 5." );
	mDBChannelInterface[5]->SetChannel( mDBChannel[5] );

	mDBChannelInterface[6].reset( new AnalyzerSettingInterfaceChannel() );
	mDBChannelInterface[6]->SetTitleAndTooltip( "DB6 Signal", "DB6 is the data bit 6." );
	mDBChannelInterface[6]->SetChannel( mDBChannel[6] );

	mDBChannelInterface[7].reset( new AnalyzerSettingInterfaceChannel() );
	mDBChannelInterface[7]->SetTitleAndTooltip( "DB7 Signal", "DB7 is the data bit 7." );
	mDBChannelInterface[7]->SetChannel( mDBChannel[7] );

  mMarkTimingErrorsInterface.reset( new AnalyzerSettingInterfaceBool() );
	mMarkTimingErrorsInterface->SetTitleAndTooltip( "Mark Timing Errors", "Determines if timing errors will be marked." );
	mMarkTimingErrorsInterface->SetValue( mMarkTimingErrors );

	mEnableCycleMinInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mEnableCycleMinInterface->SetTitleAndTooltip( "Enable Cycle Min (nS)",  "Enable cycle minimum in nS, Tcyce in datasheet." );
	mEnableCycleMinInterface->SetMax( 999999 );
	mEnableCycleMinInterface->SetMin( 0 );
	mEnableCycleMinInterface->SetInteger( mEnableCycleMin );

	mEnablePulseWidthMinInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mEnablePulseWidthMinInterface->SetTitleAndTooltip( "Enable Pulse Width Min (nS)",  "Enable pulse width minimum in nS, PWeh in datasheet." );
	mEnablePulseWidthMinInterface->SetMax( 999999 );
	mEnablePulseWidthMinInterface->SetMin( 0 );
	mEnablePulseWidthMinInterface->SetInteger( mEnablePulseWidthMin);

	mAddressSetupMinInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mAddressSetupMinInterface->SetTitleAndTooltip( "Address Setup Min (nS)",  "Address setup minimum in nS, Tas in data sheet." );
	mAddressSetupMinInterface->SetMax( 999999 );
	mAddressSetupMinInterface->SetMin( 0 );
	mAddressSetupMinInterface->SetInteger( mAddressSetupMin );

	mAddressHoldMinInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mAddressHoldMinInterface->SetTitleAndTooltip( "Address Hold Min (nS)",  "Address hold minimum in nS, Tah in data sheet." );
	mAddressHoldMinInterface->SetMax( 999999 );
	mAddressHoldMinInterface->SetMin( 0 );
	mAddressHoldMinInterface->SetInteger( mAddressHoldMin );

	mDataWriteSetupMinInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mDataWriteSetupMinInterface->SetTitleAndTooltip( "Data Write Setup Min (nS)",  "Data write setup minimum in nS, Tdsw in data sheet." );
	mDataWriteSetupMinInterface->SetMax( 999999 );
	mDataWriteSetupMinInterface->SetMin( 0 );
	mDataWriteSetupMinInterface->SetInteger( mDataWriteSetupMin );

	mDataWriteHoldMinInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mDataWriteHoldMinInterface->SetTitleAndTooltip( "Data Write Hold Min (nS)",  "Data write hold minimum in nS, Th in data sheet." );
	mDataWriteHoldMinInterface->SetMax( 999999 );
	mDataWriteHoldMinInterface->SetMin( 0 );
	mDataWriteHoldMinInterface->SetInteger( mDataWriteHoldMin );

	mDataReadDelayMaxInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mDataReadDelayMaxInterface->SetTitleAndTooltip( "Data Read Delay Max (nS)",  "Data read delay maximum in nS, Tddr in data sheet." );
	mDataReadDelayMaxInterface->SetMax( 999999 );
	mDataReadDelayMaxInterface->SetMin( 0 );
	mDataReadDelayMaxInterface->SetInteger( mDataReadDelayMax );

	mDataReadHoldMinInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mDataReadHoldMinInterface->SetTitleAndTooltip( "Data Read Hold Min (nS)",  "Data read hold minimum in nS, Tdhr in data sheet." );
	mDataReadHoldMinInterface->SetMax( 999999 );
	mDataReadHoldMinInterface->SetMin( 0 );
	mDataReadHoldMinInterface->SetInteger( mDataReadHoldMin );

  mIgnoreEPulsesWhenBusyInterface.reset( new AnalyzerSettingInterfaceBool() );
	mIgnoreEPulsesWhenBusyInterface->SetTitleAndTooltip( "Ignore E Pulses When LCD Controller is Busy", "Determines if E pulses will be ignored when busy." );
	mIgnoreEPulsesWhenBusyInterface->SetValue( mIgnoreEPulsesWhenBusy );

	mBusyTimeClearHomeInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBusyTimeClearHomeInterface->SetTitleAndTooltip( "Busy Time for Clear/Home (uS)",  "Busy time for clear display and return home in uS." );
	mBusyTimeClearHomeInterface->SetMax( 999999 );
	mBusyTimeClearHomeInterface->SetMin( 0 );
	mBusyTimeClearHomeInterface->SetInteger( mBusyTimeClearHome );

	mBusyTimeCmdCharInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBusyTimeCmdCharInterface->SetTitleAndTooltip( "Busy Time for Command/Data (uS)",  "Busy time for command and data reading or writing in uS." );
	mBusyTimeCmdCharInterface->SetMax( 999999 );
	mBusyTimeCmdCharInterface->SetMin( 0 );
	mBusyTimeCmdCharInterface->SetInteger( mBusyTimeCmdChar );

  mStartIn4BitModeInterface.reset( new AnalyzerSettingInterfaceBool() );
	mStartIn4BitModeInterface->SetTitleAndTooltip( "Start In 4-bit Mode (Init Missing)","Select this option for 4-bit mode when missing a proper init sequence." );
	mStartIn4BitModeInterface->SetValue( mStartIn4BitMode );

  mDoNotGenerateBusyCheckFramesInterface.reset( new AnalyzerSettingInterfaceBool() );
	mDoNotGenerateBusyCheckFramesInterface->SetTitleAndTooltip( "Do Not Generate Frames for Busy Checks","Select this option to not generate frames for busy checks." );
	mDoNotGenerateBusyCheckFramesInterface->SetValue( mDoNotGenerateBusyCheckFrames);

  //add interfaces
	AddInterface( mEChannelInterface.get() );
	AddInterface( mRSChannelInterface.get() );
	AddInterface( mRWChannelInterface.get() );
  for (dbline=0;dbline<8;dbline++)
    AddInterface( mDBChannelInterface[dbline].get() );
	AddInterface( mMarkTimingErrorsInterface.get() );
	AddInterface( mEnableCycleMinInterface.get() );
	AddInterface( mEnablePulseWidthMinInterface.get() );
	AddInterface( mAddressSetupMinInterface.get() );
	AddInterface( mAddressHoldMinInterface.get() );
	AddInterface( mDataWriteSetupMinInterface.get() );
	AddInterface( mDataWriteHoldMinInterface.get() );
	AddInterface( mDataReadDelayMaxInterface.get() );
	AddInterface( mDataReadHoldMinInterface.get() );
	AddInterface( mIgnoreEPulsesWhenBusyInterface.get() );
	AddInterface( mBusyTimeClearHomeInterface.get() );
	AddInterface( mBusyTimeCmdCharInterface.get() );
	AddInterface( mStartIn4BitModeInterface.get() );
	AddInterface( mDoNotGenerateBusyCheckFramesInterface.get() );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

  //clear and add channels
  ClearAndAddChannels();
}

HD44780AnalyzerSettings::~HD44780AnalyzerSettings()
{
}

void HD44780AnalyzerSettings::ClearAndAddChannels()
{
  char s1[8];
  U32 dbline;

  //clear and add channels  
	ClearChannels();
	AddChannel( mEChannel, "E", mEChannel != UNDEFINED_CHANNEL);
	AddChannel( mRSChannel, "RS", mRSChannel != UNDEFINED_CHANNEL);
	AddChannel( mRWChannel, "RW", mRWChannel != UNDEFINED_CHANNEL);

  for (dbline=0;dbline<8;dbline++)
    {
      sprintf(s1,"DB%d",dbline);
      AddChannel( mDBChannel[dbline],s1, mDBChannel[dbline] != UNDEFINED_CHANNEL);
    }
}

bool HD44780AnalyzerSettings::SetSettingsFromInterfaces()
{
	std::vector<Channel> channels;
  U32 dbline,count;

  //is a proper 4-bit or 8-bit mode selected?
  for (dbline=0,count=0;dbline<4;dbline++)
    if (mDBChannelInterface[dbline]->GetChannel() == UNDEFINED_CHANNEL)
      count++;
  if (count!=0 && count!=4)
    {
      SetErrorText( "DB0-DB3 must all be set to none for 4-bit, or must all have a valid channel for 8-bit." );
  		return false;
    }

  //do any channels overlap?
	channels.push_back( mEChannelInterface->GetChannel() );
	channels.push_back( mRSChannelInterface->GetChannel() );
	channels.push_back( mRWChannelInterface->GetChannel() );
  for (dbline=0;dbline<8;dbline++)
    channels.push_back( mDBChannelInterface[dbline]->GetChannel() );
	if( AnalyzerHelpers::DoChannelsOverlap( &channels[0], channels.size() ) == true )
  	{
	  	SetErrorText( "Please select different channels for each input." );
  		return false;
	  }

  //valiation passed

  //set settings
  mEChannel=mEChannelInterface->GetChannel();
  mRSChannel=mRSChannelInterface->GetChannel();
  mRWChannel=mRWChannelInterface->GetChannel();
  for (dbline=0;dbline<8;dbline++)
    mDBChannel[dbline]=mDBChannelInterface[dbline]->GetChannel();
  mMarkTimingErrors=mMarkTimingErrorsInterface->GetValue();
  mEnableCycleMin=mEnableCycleMinInterface->GetInteger();
  mEnablePulseWidthMin=mEnablePulseWidthMinInterface->GetInteger();
  mAddressSetupMin=mAddressSetupMinInterface->GetInteger();
  mAddressHoldMin=mAddressHoldMinInterface->GetInteger();
  mDataWriteSetupMin=mDataWriteSetupMinInterface->GetInteger();
  mDataWriteHoldMin=mDataWriteHoldMinInterface->GetInteger();
  mDataReadDelayMax=mDataReadDelayMaxInterface->GetInteger();
  mDataReadHoldMin=mDataReadHoldMinInterface->GetInteger();
  mIgnoreEPulsesWhenBusy=mIgnoreEPulsesWhenBusyInterface->GetValue();
  mBusyTimeClearHome=mBusyTimeClearHomeInterface->GetInteger();
  mBusyTimeCmdChar=mBusyTimeCmdCharInterface->GetInteger();
  mStartIn4BitMode=mStartIn4BitModeInterface->GetValue();
  mDoNotGenerateBusyCheckFrames=mDoNotGenerateBusyCheckFramesInterface->GetValue();

  //clear and add channels
  ClearAndAddChannels();

	return true;
}

void HD44780AnalyzerSettings::UpdateInterfacesFromSettings()
{
  U32 dbline;

  //settings -> interfaces
  mEChannelInterface->SetChannel(mEChannel);
  mRSChannelInterface->SetChannel(mRSChannel);
  mRWChannelInterface->SetChannel(mRWChannel);

  for (dbline=0;dbline<8;dbline++)
    mDBChannelInterface[dbline]->SetChannel(mDBChannel[dbline]);
  mMarkTimingErrorsInterface->SetValue(mMarkTimingErrors);
  mEnableCycleMinInterface->SetInteger(mEnableCycleMin);
  mEnablePulseWidthMinInterface->SetInteger(mEnablePulseWidthMin);
  mAddressSetupMinInterface->SetInteger(mAddressSetupMin);
  mAddressHoldMinInterface->SetInteger(mAddressHoldMin);
  mDataWriteSetupMinInterface->SetInteger(mDataWriteSetupMin);
  mDataWriteHoldMinInterface->SetInteger(mDataWriteHoldMin);
  mDataReadDelayMaxInterface->SetInteger(mDataReadDelayMax);
  mDataReadHoldMinInterface->SetInteger(mDataReadHoldMin);
  mIgnoreEPulsesWhenBusyInterface->SetValue(mIgnoreEPulsesWhenBusy);
  mBusyTimeClearHomeInterface->SetInteger(mBusyTimeClearHome);
  mBusyTimeCmdCharInterface->SetInteger(mBusyTimeCmdChar);
  mStartIn4BitModeInterface->SetValue(mStartIn4BitMode);
  mDoNotGenerateBusyCheckFramesInterface->SetValue(mDoNotGenerateBusyCheckFrames);
}

void HD44780AnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	const char* name_string;	//the first thing in the archive is the name of the protocol analyzer that the data belongs to.
  U32 version,dbline;

  //load settings into text_archive
  text_archive.SetString( settings );

  //settings check
	text_archive >> &name_string;
	if( strcmp( name_string, ANALYZER_SAVE_NAME ) != 0 )
		AnalyzerHelpers::Assert( "Analyzer loadsettings provided with incorrect save name." );

  //version check
  text_archive >> version;
  if (version>ANALYZER_SAVE_VERSION)
    {
      //instead of asserting let's just skip loading and allow the application to continue with default values
      return;
    }

  //code can be added here to see if an older version is being loaded someday when an older version might possibly exist

  //load settings
  text_archive >> mEChannel;
  text_archive >> mRSChannel;
  text_archive >> mRWChannel;
  for (dbline=0;dbline<8;dbline++)
    text_archive >> mDBChannel[dbline];
  text_archive >> mMarkTimingErrors;
  text_archive >> mEnableCycleMin;
  text_archive >> mEnablePulseWidthMin;
  text_archive >> mAddressSetupMin;
  text_archive >> mAddressHoldMin;
  text_archive >> mDataWriteSetupMin;
  text_archive >> mDataWriteHoldMin;
  text_archive >> mDataReadDelayMax;
  text_archive >> mDataReadHoldMin;
  text_archive >> mIgnoreEPulsesWhenBusy;
  text_archive >> mBusyTimeClearHome;
  text_archive >> mBusyTimeCmdChar;
	text_archive >> mStartIn4BitMode;
  text_archive >> mDoNotGenerateBusyCheckFrames;

  //clear and add channels
  ClearAndAddChannels();

  //set interfaces to these settings
	UpdateInterfacesFromSettings();
}

const char* HD44780AnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;
  U32 dbline;

  //save settings
	text_archive << ANALYZER_SAVE_NAME;
  text_archive << ANALYZER_SAVE_VERSION;
  text_archive << mEChannel;
  text_archive << mRSChannel;
  text_archive << mRWChannel;
  for (dbline=0;dbline<8;dbline++)
    text_archive << mDBChannel[dbline];
  text_archive << mMarkTimingErrors;
  text_archive << mEnableCycleMin;
  text_archive << mEnablePulseWidthMin;
  text_archive << mAddressSetupMin;
  text_archive << mAddressHoldMin;
  text_archive << mDataWriteSetupMin;
  text_archive << mDataWriteHoldMin;
  text_archive << mDataReadDelayMax;
  text_archive << mDataReadHoldMin;
  text_archive << mIgnoreEPulsesWhenBusy;
  text_archive << mBusyTimeClearHome;
  text_archive << mBusyTimeCmdChar;
  text_archive << mStartIn4BitMode;
  text_archive << mDoNotGenerateBusyCheckFrames;

	return SetReturnString( text_archive.GetString() );
}

