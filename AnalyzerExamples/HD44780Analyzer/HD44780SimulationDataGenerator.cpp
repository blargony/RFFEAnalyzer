#include "HD44780SimulationDataGenerator.h"
#include "HD44780AnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <stdio.h>
#include <string.h>


#define TIMING_MARGIN 0.25
#define LCD_MESSAGE_WRITE "Hello World"
#define LCD_MESSAGE_READ "Hello"

HD44780SimulationDataGenerator::HD44780SimulationDataGenerator()
 : value(1)
{
}

HD44780SimulationDataGenerator::~HD44780SimulationDataGenerator()
{
}

void HD44780SimulationDataGenerator::Initialize( U32 simulation_sample_rate, HD44780AnalyzerSettings* settings )
{
  U32 dbline;

  //capture sample rate and settings   
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

  //assign channel variables
  mE = mSimulationChannels.Add( settings->mEChannel, mSimulationSampleRateHz, BIT_LOW );
  mRS = mSimulationChannels.Add( settings->mRSChannel, mSimulationSampleRateHz, BIT_LOW );
	if( settings->mRWChannel != UNDEFINED_CHANNEL ) //RW is optional
		mRW = mSimulationChannels.Add( settings->mRWChannel, mSimulationSampleRateHz, BIT_LOW );
	else mRW = NULL;
  for (dbline=0;dbline<8;dbline++)
    if( settings->mDBChannel[dbline] != UNDEFINED_CHANNEL ) //some channels are optional
      mDB[dbline] = mSimulationChannels.Add( settings->mDBChannel[dbline], mSimulationSampleRateHz, BIT_LOW );
    else mDB[dbline] = NULL;
}

//converts time to samples going one more when necessary
U32 HD44780SimulationDataGenerator::TimeToSamplesOrMore(double AS)
{
  AS*=mSimulationSampleRateHz;
  if (((U32)AS)==AS)
    return (U32)AS;
  else return (U32)AS+1;
}

//converts time to samples going one less when necessary
U32 HD44780SimulationDataGenerator::TimeToSamplesOrLess(double AS)
{
  AS*=mSimulationSampleRateHz;
  return (U32)AS;
}

//do operation creates one E operation (might be 8-bits or 4-bits depending)
void HD44780SimulationDataGenerator::DoOperation(bool ARS, bool ARW, U8 AData)
{
  U32 dbline,ui1,ui2;
  //RS false=low=0=command, true=high=1=data
  //RW false=low=0=write,   true=high=1=read

  //setup rs/rw
  mRS->TransitionIfNeeded(ARS?BIT_HIGH:BIT_LOW);
  if (mRW!=NULL)
    mRW->TransitionIfNeeded(ARW?BIT_HIGH:BIT_LOW);

  //delay address setup
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mAddressSetupMin*(1+TIMING_MARGIN)/_NS) );

  //take e high
  mE->TransitionIfNeeded(BIT_HIGH);

  if (ARW)
    {
      //read
      mSimulationChannels.AdvanceAll( TimeToSamplesOrLess( mSettings->mDataReadDelayMax*(1-TIMING_MARGIN)/_NS) );

      for (dbline=0;dbline<8;dbline++)
        if (mDB[dbline]!=NULL)
          mDB[dbline]->TransitionIfNeeded((AData & (1<<dbline))?BIT_HIGH:BIT_LOW);

      ui1=TimeToSamplesOrLess( mSettings->mDataReadDelayMax*(1-TIMING_MARGIN)/_NS);
      ui2=TimeToSamplesOrLess( mSettings->mEnablePulseWidthMin*(1+TIMING_MARGIN)/_NS);
      if (ui2>ui1)
        mSimulationChannels.AdvanceAll(ui2-ui1);
    }
  else
    {
      //write
      ui1=TimeToSamplesOrMore( mSettings->mDataWriteSetupMin*(1+TIMING_MARGIN)/_NS);
      ui2=TimeToSamplesOrMore( mSettings->mEnablePulseWidthMin*(1+TIMING_MARGIN)/_NS);
      if (ui2>ui1)
        mSimulationChannels.AdvanceAll(ui2-ui1);

      for (dbline=0;dbline<8;dbline++)
        if (mDB[dbline]!=NULL)
          mDB[dbline]->TransitionIfNeeded((AData & (1<<dbline))?BIT_HIGH:BIT_LOW);

      mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mDataWriteSetupMin*(1+TIMING_MARGIN)/_NS) );
    }

  //take e low
  mE->TransitionIfNeeded(BIT_LOW);

  //delay hold time
  ui1=TimeToSamplesOrMore( (mSettings->mEnablePulseWidthMin)*(1+TIMING_MARGIN)/_NS);
  ui2=TimeToSamplesOrMore( (mSettings->mEnableCycleMin)*(1+TIMING_MARGIN)/_NS);
  if (ui2>ui1)
    mSimulationChannels.AdvanceAll(ui2-ui1);
}

//do transfer creates one transfer
void HD44780SimulationDataGenerator::DoTransfer(bool ARS, bool ARW, U8 AData)
{
  if (bitmode8)
    {
      //8-bit mode
      DoOperation(ARS,ARW,AData);
    }
  else
    {
      //4-bit mode
      DoOperation(ARS,ARW,AData);
      DoOperation(ARS,ARW,AData<<4);
    }
}

//creates an init sequence
void HD44780SimulationDataGenerator::Init()
{
  bitmode8=true;

  //wait 40ms
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( LCD_INIT1_DELAY ) );

  //send first init and wait 4.1ms
  DoTransfer(false,false,_BV(LCD_FUNCTION) | _BV(LCD_FUNCTION_8BIT));
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( LCD_INIT2_DELAY ) );

  //send second init and wait 100us
  DoTransfer(false,false,_BV(LCD_FUNCTION) | _BV(LCD_FUNCTION_8BIT));
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( LCD_INIT3_DELAY ) );

  //send third init and wait command busy time
  DoTransfer(false,false,_BV(LCD_FUNCTION) | _BV(LCD_FUNCTION_8BIT));
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeCmdChar*(1+TIMING_MARGIN)/_US) );

  //switch to 4-bit mode if necessary
  if (mDB[0]==NULL)
    {
      DoTransfer(false,false,_BV(LCD_FUNCTION));
      mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeCmdChar*(1+TIMING_MARGIN)/_US) );

      bitmode8=false;
    }

  //function set
  DoTransfer(false,false,_BV(LCD_FUNCTION) | _BV(LCD_FUNCTION_2LINES) | (bitmode8?_BV(LCD_FUNCTION_8BIT):0));

  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeCmdChar*(1+TIMING_MARGIN)/_US) );

  //display off
  DoTransfer(false,false,_BV(LCD_DISPLAYMODE));
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeCmdChar*(1+TIMING_MARGIN)/_US) );

  //display clear
  DoTransfer(false,false,_BV(LCD_CLR));
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeClearHome*(1+TIMING_MARGIN)/_US) );

  //entry mode set
  DoTransfer(false,false,_BV(LCD_ENTRY_MODE) | _BV(LCD_ENTRY_INC));
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeCmdChar*(1+TIMING_MARGIN)/_US) );

  //display on/cursor on
  DoTransfer(false,false,_BV(LCD_DISPLAYMODE) | _BV(LCD_DISPLAYMODE_ON) | _BV(LCD_DISPLAYMODE_CURSOR));
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeCmdChar*(1+TIMING_MARGIN)/_US) );
}

//transfers a string
void HD44780SimulationDataGenerator::DoTransferString(bool ARW, char *AString)
{
  U32 i1,i2;

  for (i1=0,i2=strlen(AString);i1<i2;i1++)
    {
      DoTransfer(true,ARW,AString[i1]);
      mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeCmdChar*(1+TIMING_MARGIN)/_US) );
    }
}

//creates an output sequence (sometimes input if RW is present)
void HD44780SimulationDataGenerator::Output()
{
  char s1[256];

  //display clear
  DoTransfer(false,false,_BV(LCD_CLR));
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeClearHome*(1+TIMING_MARGIN)/_US) );

  //write data ("Hello World")
  strcpy(s1,LCD_MESSAGE_WRITE);
  DoTransferString(false,s1);

  //if (rw) read busy flag/address
  if (mRW!=NULL)
    {
      DoTransfer(false,true,strlen(LCD_MESSAGE_WRITE));
      mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeCmdChar*(1+TIMING_MARGIN)/_US) );
    }

  //set ddram addr 15
  DoTransfer(false,false,_BV(LCD_DDRAM) | 15);
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeCmdChar*(1+TIMING_MARGIN)/_US) );

  //write data (incrementing integer)
  sprintf(s1,"%u",value++);
  DoTransferString(false,s1);

  //if (rw) set ddram addr 0, read 5 characters
  if (mRW!=NULL)
    {
      DoTransfer(false,false,_BV(LCD_DDRAM));
      mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( mSettings->mBusyTimeCmdChar*(1+TIMING_MARGIN)/_US) );

      strcpy(s1,LCD_MESSAGE_READ);
      DoTransferString(true,s1);
    }

  //delay 1ms
  mSimulationChannels.AdvanceAll( TimeToSamplesOrMore( 1/_MS ) );
}

U32 HD44780SimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mE->GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
    if (!mE->GetCurrentSampleNumber())
      Init();

    Output();
	}

	*simulation_channels = mSimulationChannels.GetArray();
	return mSimulationChannels.GetCount();
}

