#include "RFFEAnalyzer.h"
#include "RFFEAnalyzerSettings.h"
#include "RFFEUtil.h"
#include <AnalyzerChannelData.h>


RFFEAnalyzer::RFFEAnalyzer()
:	Analyzer2(),  
	mSettings( new RFFEAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

RFFEAnalyzer::~RFFEAnalyzer()
{
	KillThread();
}

void RFFEAnalyzer::SetupResults()
{
	mResults.reset( new RFFEAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mSdataChannel );
}

void RFFEAnalyzer::WorkerThread()
{
    S32 count;
	mSampleRateHz = GetSampleRate();

	mSdata = GetAnalyzerChannelData( mSettings->mSdataChannel );
	mSclk  = GetAnalyzerChannelData( mSettings->mSclkChannel );

    mResults->CancelPacketAndStartNewPacket();

	for( ; ; )
	{
        count = FindStartSeqCondition();
        if ( count == -1 )
        {
            mResults->CancelPacketAndStartNewPacket();
            break;
        }
        count = FindSlaveAddrAndCommand();
        if ( count == -1 )
        {
            mResults->CancelPacketAndStartNewPacket();
            continue;
        }
        FindParity(true);

        switch ( mRffeType )
        {
        case RFFEAnalyzerResults::RffeTypeExtWrite:
            FindAddressFrame( RFFEAnalyzerResults::RffeAddressNormalField );
            for( U32 i = count ; i != 0; i-- )
            {
                FindDataFrame();
            }
            FindBusPark();
            break;
        case RFFEAnalyzerResults::RffeTypeReserved:
            break;
        case RFFEAnalyzerResults::RffeTypeExtRead:
            FindAddressFrame( RFFEAnalyzerResults::RffeAddressNormalField );
            FindBusPark();
            for( U32 i = count ; i != 0; i-- )
            {
                FindDataFrame();
            }
            FindBusPark();
            break;
        case RFFEAnalyzerResults::RffeTypeExtLongWrite:
            FindAddressFrame( RFFEAnalyzerResults::RffeAddressHiField );
            FindAddressFrame( RFFEAnalyzerResults::RffeAddressLoField );
            for( U32 i = count ; i != 0; i-- )
            {
                FindDataFrame();
            }
            FindBusPark();
            break;
        case RFFEAnalyzerResults::RffeTypeExtLongRead:
            FindAddressFrame( RFFEAnalyzerResults::RffeAddressHiField );
            FindAddressFrame( RFFEAnalyzerResults::RffeAddressLoField );
            FindBusPark();
            for( U32 i = count ; i != 0; i-- )
            {
                FindDataFrame();
            }
            FindBusPark();
            break;
        case RFFEAnalyzerResults::RffeTypeNormalWrite:
            FindDataFrame();
            FindBusPark();
            break;
        case RFFEAnalyzerResults::RffeTypeNormalRead:
            FindBusPark();
            FindDataFrame();
            FindBusPark();
            break;
        case RFFEAnalyzerResults::RffeTypeShortWrite:
            FindBusPark();
            break;
        }
        mResults->CommitPacketAndStartNewPacket();
        CheckIfThreadShouldExit();
	}
}

void RFFEAnalyzer::AdvanceToBeginningStartBit()
{
    U64 sample;
    BitState state;

	for( ; ; )
	{
		mSdata->AdvanceToNextEdge();
        state = mSdata->GetBitState();
		if( state == BIT_HIGH )
		{
            sample = mSdata->GetSampleNumber();
			mSclk->AdvanceToAbsPosition( sample );
            state  = mSclk->GetBitState();
			if( state == BIT_LOW )
				break;
		}	
	}
}

S32 RFFEAnalyzer::FindStartSeqCondition()
{
    U64 sample;
    BitState state;

    U64 sample_up;
    U64 sample_dn;
    U64 sample_next;
    U64 pulse_1, pulse_2;
    bool did_toggle;

    // in case that clk is ahead of data
    sample_up = mSdata->GetSampleNumber();
    sample_dn =  mSclk->GetSampleNumber();
    if ( sample_dn > sample_up )
    {
        mSdata->AdvanceToAbsPosition( sample_dn );
    }

    for ( ; ; )
    {
        if( ! mSclk->DoMoreTransitionsExistInCurrentData() ) return -1;
        if( ! mSdata->DoMoreTransitionsExistInCurrentData() ) return -1;

        sample = mSdata->GetSampleNumber();
        mSclk->AdvanceToAbsPosition( sample );
        state  = mSdata->GetBitState();
        if ( state == BIT_LOW )
        {
            AdvanceToBeginningStartBit();
        }
        else
        {
            mSdata->AdvanceToNextEdge();
            continue;
        }

        // advance the pulse
        sample_up = mSdata->GetSampleNumber();
        mSdata->AdvanceToNextEdge();
        sample_dn = mSdata->GetSampleNumber();
        pulse_1   = sample_dn - sample_up;

        // use to scan for more clocks ahead, in samples
        pulse_width2 = pulse_1 * 2;

        did_toggle = mSclk->WouldAdvancingToAbsPositionCauseTransition( sample_dn );
        if( did_toggle )
            continue; // error: found clk toggling

        // look for idle in clk & data signals
        mSclk->AdvanceToAbsPosition( sample_dn );
        sample_next = mSclk->GetSampleOfNextEdge();
        pulse_2     = sample_next - sample_dn;

        if ( pulse_2 < pulse_1 )
            continue; // error: idle period too short

        // at rising edge of clk
        mSclk->AdvanceToNextEdge();
        sample = mSclk->GetSampleNumber();
        mSdata->AdvanceToAbsPosition( sample );

		Frame frame;
        frame.mType                    = RFFEAnalyzerResults::RffeSSCField;
		frame.mStartingSampleInclusive = sample_up;
		frame.mEndingSampleInclusive   = sample;

        mResults->AddMarker( sample_up,
                             AnalyzerResults::Start,
                             mSettings->mSdataChannel );
		mResults->AddFrame( frame );
		mResults->CommitResults();

		ReportProgress( frame.mEndingSampleInclusive );
        break;
    }

    gSampleCount = 0;
    return 1;
}

bool RFFEAnalyzer::CheckClockRate()
{
    U32 average;
    U32 pulse;
    U32 pulse_lo;

    average = (U32)(gSampleClk[11]/12);
    pulse   = (U32)(pulse_width2/2);
    pulse_lo= pulse - 1;
    if ( pulse == 1 ) return false;

    if ( (average < pulse_lo) || (average >  pulse) )
    {
            return false;
    }
    return true;
}


S32 RFFEAnalyzer::FindSlaveAddrAndCommand()
{
    S32 count = 0;
    U64 SAdr;
    U64 cmd;
    AnalyzerResults::MarkerType sampleDataState[16];

    // normalize samples for debugging
    gSampleNormalized = mSclk->GetSampleNumber();

    // starting at rising edge of clk
    cmd = GetBitStream( 12, sampleDataState);

    // check if clk rate is consistent
    if ( !CheckClockRate() )
    {
            FillInFrame( RFFEAnalyzerResults::RffeErrorCaseField,
                         gSampleClk[11],
                         pulse_width2,
                         sampleClkOffsets[0], sampleClkOffsets[12],
                         0, 4,
                         sampleDataState );
            return -1;
    }

    SAdr = ( cmd & 0xF00 ) >> 8;
    FillInFrame( RFFEAnalyzerResults::RffeSAField,
                 SAdr,
                 0,
                 sampleClkOffsets[0], sampleClkOffsets[4],
                 0, 4,
                 sampleDataState );

	// decode type
    mRffeType = RFFEUtil::decodeRFFECmdFrame( (U8)(cmd & 0xFF) );
    switch ( mRffeType )
    {
    case RFFEAnalyzerResults::RffeTypeExtWrite:
        FillInFrame( RFFEAnalyzerResults::RffeTypeField,
                     mRffeType,
                     0,
                     sampleClkOffsets[4], sampleClkOffsets[8],
                     4, 4,
                     sampleDataState );
        FillInFrame( RFFEAnalyzerResults::RffeExByteCountField,
                     ( cmd & 0x0F ),
                     0,
                     sampleClkOffsets[8], sampleClkOffsets[12],
                     8, 4,
                     sampleDataState );
        count = RFFEUtil::byteCount( (U8)cmd );
        break;
    case RFFEAnalyzerResults::RffeTypeReserved: 
        FillInFrame( RFFEAnalyzerResults::RffeTypeField,
                     mRffeType,
                     0,
                     sampleClkOffsets[4], sampleClkOffsets[12],
                     4, 8,
                     sampleDataState );
        break;
    case RFFEAnalyzerResults::RffeTypeExtRead:
        FillInFrame( RFFEAnalyzerResults::RffeTypeField,
                     mRffeType,
                     0,
                     sampleClkOffsets[4], sampleClkOffsets[8],
                     4, 4,
                     sampleDataState );
        FillInFrame( RFFEAnalyzerResults::RffeExByteCountField,
                     ( cmd & 0x0F ),
                     0,
                     sampleClkOffsets[8], sampleClkOffsets[12],
                     8, 4,
                     sampleDataState );
        count = RFFEUtil::byteCount( (U8)cmd );
        break;
    case RFFEAnalyzerResults::RffeTypeExtLongWrite:
        FillInFrame( RFFEAnalyzerResults::RffeTypeField,
                     mRffeType,
                     0,
                     sampleClkOffsets[4], sampleClkOffsets[9],
                     4, 5,
                     sampleDataState );
        FillInFrame( RFFEAnalyzerResults::RffeExLongByteCountField,
                     ( cmd & 0x07 ),
                     0,
                     sampleClkOffsets[9], sampleClkOffsets[12],
                     9, 3,
                     sampleDataState );
        count = RFFEUtil::byteCount( (U8)cmd );
        break;
    case RFFEAnalyzerResults::RffeTypeExtLongRead:
        FillInFrame( RFFEAnalyzerResults::RffeTypeField,
                     mRffeType,
                     0,
                     sampleClkOffsets[4], sampleClkOffsets[9],
                     4, 5,
                     sampleDataState );
        FillInFrame( RFFEAnalyzerResults::RffeExLongByteCountField,
                     ( cmd & 0x07 ),
                     0,
                     sampleClkOffsets[9], sampleClkOffsets[12],
                     9, 3,
                     sampleDataState );
        count = RFFEUtil::byteCount( (U8)cmd );
        break;
    case RFFEAnalyzerResults::RffeTypeNormalWrite:
        FillInFrame( RFFEAnalyzerResults::RffeTypeField,
                     mRffeType,
                     0,
                     sampleClkOffsets[4], sampleClkOffsets[7],
                     4, 3,
                     sampleDataState );
        FillInFrame( RFFEAnalyzerResults::RffeShortAddressField,
                     ( cmd & 0x1F ),
                     0,
                     sampleClkOffsets[7], sampleClkOffsets[12],
                     7, 5,
                     sampleDataState );
        break;
    case RFFEAnalyzerResults::RffeTypeNormalRead:
        FillInFrame( RFFEAnalyzerResults::RffeTypeField,
                     mRffeType,
                     0,
                     sampleClkOffsets[4], sampleClkOffsets[7],
                     4, 3,
                     sampleDataState );
        FillInFrame( RFFEAnalyzerResults::RffeShortAddressField,
                     ( cmd & 0x1F ),
                     0,
                     sampleClkOffsets[7], sampleClkOffsets[12],
                     7, 5,
                     sampleDataState );
        break;
    case RFFEAnalyzerResults::RffeTypeShortWrite:
        FillInFrame( RFFEAnalyzerResults::RffeTypeField,
                     mRffeType,
                     0,
                     sampleClkOffsets[4], sampleClkOffsets[5],
                     4, 1,
                     sampleDataState );
        FillInFrame( RFFEAnalyzerResults::RffeShortDataField,
                     ( cmd & 0x7F ) >> 7,
                     0,
                     sampleClkOffsets[5], sampleClkOffsets[12],
                     5, 7,
                     sampleDataState );
        break;
    }

    return count+1;
}

void RFFEAnalyzer::FindParity(bool fromCommandFrame)
{
    U64 data;
    BitState bitstate;
    AnalyzerResults::MarkerType state;

    bitstate = GetNextBit( 0, sampleClkOffsets, sampleDataOffsets );
    sampleClkOffsets[1] = mSclk->GetSampleNumber();
    mSdata->AdvanceToAbsPosition( sampleClkOffsets[1] );

    if ( bitstate == BIT_HIGH )
    {
        data = 1;
        state = AnalyzerResults::One;
    }
    else
    {
        data = 0;
        state = AnalyzerResults::Zero;
    }

    FillInFrame( RFFEAnalyzerResults::RffeParityField,
                 data,
                 (fromCommandFrame ? 1 : 0),
                 sampleClkOffsets[0],
                 sampleClkOffsets[1],
                 0, 1,
                 &state);
}

void RFFEAnalyzer::FindBusPark()
{
    U64 delta;
    AnalyzerResults::MarkerType mark = AnalyzerResults::Stop;

    // at rising edge of clk
    sampleClkOffsets[0] = mSclk->GetSampleNumber();
    mSclk->AdvanceToNextEdge();

    // at falling edge of clk
    sampleDataOffsets[0] = mSclk->GetSampleNumber();
    mSdata->AdvanceToAbsPosition( sampleDataOffsets[0] );
    
    // look if next rising edge is in reach
    delta =  sampleDataOffsets[0] - sampleClkOffsets[0];
    if ( mSclk->WouldAdvancingCauseTransition( (U32)(delta + 2) ) )
    {
        mSclk->AdvanceToNextEdge();
        sampleClkOffsets[1] = mSclk->GetSampleNumber();
    }
    else
    {
        sampleClkOffsets[1] = sampleDataOffsets[0] + delta;
    }

    FillInFrame( RFFEAnalyzerResults::RffeBusParkField,
                 0,
                 0,
                 sampleClkOffsets[0],
                 sampleClkOffsets[1],
                 0, 1,
                 &mark );
}

void RFFEAnalyzer::FindDataFrame()
{
    AnalyzerResults::MarkerType sampleDataState[16];

    U64 addr = GetBitStream( 8, sampleDataState );

    // decode data
    FillInFrame( RFFEAnalyzerResults::RffeDataField,
                 addr,
                 0,
                 sampleClkOffsets[0],
                 sampleClkOffsets[8],
                 0, 8,
                 sampleDataState );

    FindParity(false);
}

void RFFEAnalyzer::FindAddressFrame(RFFEAnalyzerResults::RffeAddressFieldSubType type)
{
    AnalyzerResults::MarkerType sampleDataState[16];

    U64 addr = GetBitStream( 8, sampleDataState );

    // decode address
    FillInFrame( RFFEAnalyzerResults::RffeAddressField,
                 addr,
                 type,
                 sampleClkOffsets[0],
                 sampleClkOffsets[8],
                 0, 8,
                 sampleDataState );

    FindParity(false);
}

/******************************************************************* markers */
void RFFEAnalyzer::DrawMarkersDotsAndStates( U32 start,
                                             U32 len,
                                             AnalyzerResults::MarkerType type,
                                             AnalyzerResults::MarkerType *states)
{
    for (U32 i=start; len--; i++ )
    {
        mResults->AddMarker( sampleClkOffsets[i],
                             type,
                             mSettings->mSclkChannel );
        mResults->AddMarker( sampleDataOffsets[i],
                             states[i],
                             mSettings->mSdataChannel );
    }
}


void RFFEAnalyzer::FillInFrame( RFFEAnalyzerResults::RffeFrameType type,
                                U64 frame_data1,
                                U64 frame_data2,
                                U64 starting_sample,
                                U64 ending_sample,
                                U32 markers_start,
                                U32 markers_len,
                                AnalyzerResults::MarkerType *states )
{
    Frame frame;

    frame.mType                    = (U8)type;
    frame.mData1                   = frame_data1;
    frame.mData2                   = frame_data2;
	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive   = ending_sample;

    if ( markers_len != 0 )
    {
        DrawMarkersDotsAndStates( markers_start,
                                  markers_len,
                                  AnalyzerResults::UpArrow,
                                  states );
    }
    else
    {
    }

    mResults->AddFrame( frame );
	mResults->CommitResults();
	ReportProgress( frame.mEndingSampleInclusive );
}

/**************************************************************** bits/bytes */
BitState RFFEAnalyzer::GetNextBit(U32 const idx, U64 *const clk, U64 *const data )
{
    BitState state;

    // at rising edge of clk
    clk[idx] =  mSclk->GetSampleNumber();
    gSampleClk[gSampleCount] = clk[idx] - gSampleNormalized;

    // advance to falling edge of sclk
    mSclk->AdvanceToNextEdge();
    data[idx] =  mSclk->GetSampleNumber();
    gsampleData[gSampleCount] = data[idx] - gSampleNormalized;
    gSampleCount++;

    mSdata->AdvanceToAbsPosition( data[idx] );
    state = mSdata->GetBitState();

    // at rising edge of clk
    mSclk->AdvanceToNextEdge();

    return state;
}

U64 RFFEAnalyzer::GetBitStream(U32 len, AnalyzerResults::MarkerType *states)
{
    U64 data;
    U32 i;
    BitState state;
	DataBuilder data_builder;

    data_builder.Reset( &data, AnalyzerEnums::MsbFirst , len );

    // starting at rising edge of clk
    for( i=0; i < len; i++ )
    {
        state = GetNextBit( i, sampleClkOffsets,  sampleDataOffsets );
        data_builder.AddBit( state );

        if ( mSdata->GetBitState() == BIT_HIGH )
            states[i] = AnalyzerResults::One;
        else
            states[i] = AnalyzerResults::Zero;
    }
    sampleClkOffsets[i] =  mSclk->GetSampleNumber();

    return data;
}

bool RFFEAnalyzer::NeedsRerun()
{
	return false;
}

U32 RFFEAnalyzer::GenerateSimulationData( U64 minimum_sample_index,
                                          U32 device_sample_rate,
                                          SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index,
                                                            device_sample_rate,
                                                            simulation_channels );
}

U32 RFFEAnalyzer::GetMinimumSampleRateHz()
{
	return 50000000;
}

const char* RFFEAnalyzer::GetAnalyzerName() const
{
	return "RFFEv1.0";
}

const char* GetAnalyzerName()
{
	return "RFFEv1.0";
}

Analyzer* CreateAnalyzer()
{
	return new RFFEAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}