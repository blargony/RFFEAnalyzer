#include "HD44780Analyzer.h"
#include "HD44780AnalyzerSettings.h"
#include <AnalyzerChannelData.h>

HD44780Analyzer::HD44780Analyzer()
:	Analyzer2(),
	mSettings( new HD44780AnalyzerSettings() ),
	mSimulationInitilized( false ),
  bitmode8(true),
  mLastEStart(0),
  mWaitBusy(0)
{
	SetAnalyzerSettings( mSettings.get() );
}

HD44780Analyzer::~HD44780Analyzer()
{
	KillThread();
}

//converts time to samples going one more when necessary
U32 HD44780Analyzer::TimeToSamplesOrMore(double AS)
{
  AS*=mSampleRateHz;
  if (((U32)AS)==AS)
    return (U32)AS;
  else return (U32)AS+1;
}

//converts time to samples going one less when necessary
U32 HD44780Analyzer::TimeToSamplesOrLess(double AS)
{
  AS*=mSampleRateHz;
  return (U32)AS;
}

//advances channel to abs position while marking any transitions with an X along the way
void HD44780Analyzer::AdvanceToAbsPositionWhileMarking(AnalyzerChannelData *AACD, Channel AC, U64 APosition)
{
  if (APosition<=AACD->GetSampleNumber())
    return;

  while (mSettings->mMarkTimingErrors && AACD->WouldAdvancingToAbsPositionCauseTransition(APosition) && AACD->GetSampleOfNextEdge()<APosition)
    {
      AACD->AdvanceToNextEdge();
      mResults->AddMarker(AACD->GetSampleNumber(), AnalyzerResults::X, AC );
    }
  AACD->AdvanceToAbsPosition(APosition);
}

//get operation gets one E operation (might be 8-bits or 4-bits depending)
bool HD44780Analyzer::GetOperation(bool &ARS, bool &ARW, U8 &AData, S64 &AEStart, S64 &AEEnd, bool ASecondNibble)
{
  U64 pos;
  U32 dbline,ui1;

  //move to the E high transition
  mE->AdvanceToNextEdge();
  AEStart=mE->GetSampleNumber();

  //check between E high transitions for >= mEnableCycleMin
  if (mSettings->mMarkTimingErrors && mLastEStart && AEStart-mLastEStart<TimeToSamplesOrMore(mSettings->mEnableCycleMin/_NS))
    mResults->AddMarker(AEStart, AnalyzerResults::X, mSettings->mEChannel );
  mLastEStart=AEStart;

  //advance rs/rw to the e position less mAddressSetupMin if in 8 bit mode or if in the first nibble of 4 bit mode
  if (!ASecondNibble)
    {
      ui1=TimeToSamplesOrMore(mSettings->mAddressSetupMin/_NS);
      if (AEStart>ui1)
        {
          if (AEStart-ui1>(S64)mRS->GetSampleNumber())
            mRS->AdvanceToAbsPosition(AEStart-ui1);
          if (mRW!=NULL && AEStart-ui1>(S64)mRW->GetSampleNumber()) //mRW is optional
            mRW->AdvanceToAbsPosition(AEStart-ui1);
        }
    }

  //advance rs/rw to the estart position while marking any changes
  AdvanceToAbsPositionWhileMarking(mRS,mSettings->mRSChannel,AEStart);
  if (mRW!=NULL) //mRW is optional
    AdvanceToAbsPositionWhileMarking(mRW,mSettings->mRWChannel,AEStart);

  //sample rs/rw
  ARS=(mRS->GetBitState()==BIT_HIGH)?true:false;
  if (!ASecondNibble) //do not mark second nibble rs/rw
    mResults->AddMarker(AEStart, AnalyzerResults::Dot, mSettings->mRSChannel );
  if (mRW!=NULL) //mRW is optional
    {
      ARW=(mRW->GetBitState()==BIT_HIGH)?true:false;
      if (!ASecondNibble) //do not mark second nibble rs/rw
        mResults->AddMarker(AEStart, AnalyzerResults::Dot, mSettings->mRWChannel );
    }
  else ARW=false; //if RW not present we assume it is ground (write)

  //move to the E low transition
  mE->AdvanceToNextEdge();
  AEEnd=mE->GetSampleNumber();

  //check E pulse width for >= mEnablePulseWidthMin
  if (mSettings->mMarkTimingErrors && AEEnd-AEStart<TimeToSamplesOrMore(mSettings->mEnablePulseWidthMin/_NS))
    mResults->AddMarker(AEEnd, AnalyzerResults::X, mSettings->mEChannel );

  //determine position to advance dbX and advance dbX based on read/write
  //advance dbX to the eend position while marking any changes
  if (ARW)
    pos=AEStart+TimeToSamplesOrMore(mSettings->mDataReadDelayMax/_NS);
  else pos=AEEnd-TimeToSamplesOrMore(mSettings->mDataWriteSetupMin/_NS);
  for (dbline=0;dbline<8;dbline++)
    if (mDB[dbline]!=NULL) //some channels are optional
      {
        if (pos>mDB[dbline]->GetSampleNumber())
          mDB[dbline]->AdvanceToAbsPosition(pos);
        AdvanceToAbsPositionWhileMarking(mDB[dbline],mSettings->mDBChannel[dbline],AEEnd);
      }

  //sample dbX
  AData=0;
  for (dbline=0;dbline<8;dbline++)
    if (mDB[dbline]!=NULL) //some channels are optional
      {
        if (mDB[dbline]->GetBitState()==BIT_HIGH)
          AData|=(1<<dbline);
        mResults->AddMarker(AEEnd, AnalyzerResults::Dot, mSettings->mDBChannel[dbline] );
      }

  //advance rs/rw to the eend+mAddressHoldMin while marking any changes
  AdvanceToAbsPositionWhileMarking(mRS,mSettings->mRSChannel,AEEnd+TimeToSamplesOrMore(mSettings->mAddressHoldMin/_NS));
  if (mRW!=NULL) //mRW is optional
    AdvanceToAbsPositionWhileMarking(mRW,mSettings->mRWChannel,AEEnd+TimeToSamplesOrMore(mSettings->mAddressHoldMin/_NS));

  //advance dbX to the eend+mDataReadHoldMin (read) or eend+mDataWriteHoldMin (write)
  if (ARW)
    pos=AEEnd+TimeToSamplesOrMore(mSettings->mDataReadHoldMin/_NS);
  else pos=AEEnd+TimeToSamplesOrMore(mSettings->mDataWriteHoldMin/_NS);
  for (dbline=0;dbline<8;dbline++)
    if (mDB[dbline]!=NULL) //some channels are optional
      AdvanceToAbsPositionWhileMarking(mDB[dbline],mSettings->mDBChannel[dbline],pos);

  return true;
}

//get transfer gets one transfer
void HD44780Analyzer::GetTransfer()
{
  bool RS,RS2,RW,RW2;
  U8 Data,Data2;
  S64 EDummy;
  Frame frame;

  if (bitmode8)
    {
      //8-bit mode
      if (!GetOperation(RS,RW,Data,frame.mStartingSampleInclusive,frame.mEndingSampleInclusive,false))
        return;

      //if busy still and not a check busy command, we will mark and ignore this frame
      if ((!(RW && !RS)) && mWaitBusy && (U64)frame.mStartingSampleInclusive<mWaitBusy)
        {
          mResults->AddMarker(frame.mStartingSampleInclusive, AnalyzerResults::X, mSettings->mEChannel);
          return;
        }
    }
  else
    {
      //4-bit mode high nibble
      if (!GetOperation(RS,RW,Data,frame.mStartingSampleInclusive,EDummy,false))
        return;

      //if busy still and not a check busy command, we will mark and ignore this frame
      if ((!(RW && !RS)) && mWaitBusy && (U64)frame.mStartingSampleInclusive<mWaitBusy)
        {
          mResults->AddMarker(frame.mStartingSampleInclusive, AnalyzerResults::X, mSettings->mEChannel);
          return;
        }

      //4-bit mode low nibble
      if (!GetOperation(RS2,RW2,Data2,EDummy,frame.mEndingSampleInclusive,true))
        return;

      //recombine nibbles
      Data|=(Data2>>4);
    }

  //handle busy mode if enabled
  if (mSettings->mIgnoreEPulsesWhenBusy)
    {
      //if command causes LCD controller to be busy, calculate how long
      if (RS)
        mWaitBusy=frame.mEndingSampleInclusive+TimeToSamplesOrMore(mSettings->mBusyTimeCmdChar/_US);
      else
      if (!RW)
        {
          if (((Data & LCD_CLR_MASK)==_BV(LCD_CLR)) || ((Data & LCD_HOME_MASK)==_BV(LCD_HOME)))
            mWaitBusy=frame.mEndingSampleInclusive+TimeToSamplesOrMore(mSettings->mBusyTimeClearHome/_US);
          else mWaitBusy=frame.mEndingSampleInclusive+TimeToSamplesOrMore(mSettings->mBusyTimeCmdChar/_US);
        }

      //if check busy returns not busy cancel busy mode
      if ((RW && !RS) && !(Data & _BV(LCD_BUSY)))
        mWaitBusy=0;
    }

  //function set command switches between 4-bit and 8-bit mode
  if (!RW && !RS && ((Data & LCD_FUNCTION_MASK)==_BV(LCD_FUNCTION)))
    bitmode8=(Data & _BV(LCD_FUNCTION_8BIT))?true:false;

  //create frame
  frame.mData1 = Data;
  frame.mFlags = (RS?_BV(MFLAG_RS):0) | (RW?_BV(MFLAG_RW):0) ;

  if (!mSettings->mDoNotGenerateBusyCheckFrames || !(RW && !RS))
    mResults->AddFrame( frame );

  mResults->CommitResults();
  ReportProgress( frame.mEndingSampleInclusive );
}

void HD44780Analyzer::WorkerThread()
{
  U32 dbline;

  //get sample rate
	mSampleRateHz = GetSampleRate();

  //setup channels
	mE = GetAnalyzerChannelData( mSettings->mEChannel );
	mRS = GetAnalyzerChannelData( mSettings->mRSChannel );
	if( mSettings->mRWChannel != UNDEFINED_CHANNEL )
    mRW = GetAnalyzerChannelData( mSettings->mRWChannel );
	else mRW = NULL;
  for (dbline=0;dbline<8;dbline++)
    if( mSettings->mDBChannel[dbline] != UNDEFINED_CHANNEL )
      mDB[dbline] = GetAnalyzerChannelData( mSettings->mDBChannel[dbline] );
    else mDB[dbline] = NULL;

  //if we start high we need to move to low
	if( mE->GetBitState() == BIT_HIGH)
		mE->AdvanceToNextEdge();

  //we always start in 8-bit mode unless the force 4-bit mode option is selected
  //function set command can change modes
  bitmode8=!mSettings->mStartIn4BitMode;

  //reset between E high pulse measurements
  mLastEStart=0;

  //reset busy mode
  mWaitBusy=0;

  //get frames
	for( ; ; )
    {
      GetTransfer();
      CheckIfThreadShouldExit();
    }
}

void HD44780Analyzer::SetupResults()
{
    mResults.reset( new HD44780AnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mEChannel );
}

bool HD44780Analyzer::NeedsRerun()
{
	return false;
}

U32 HD44780Analyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 HD44780Analyzer::GetMinimumSampleRateHz()
{
	return 25000;
}

const char* HD44780Analyzer::GetAnalyzerName() const
{
	return "HD44780";
}

const char* GetAnalyzerName()
{
	return "HD44780";
}

Analyzer* CreateAnalyzer()
{
	return new HD44780Analyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
