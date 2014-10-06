#include "RFFESimulationDataGenerator.h"
#include "RFFEAnalyzerSettings.h"
#include "RFFEUtil.h"

#include <AnalyzerHelpers.h>

RFFESimulationDataGenerator::RFFESimulationDataGenerator()
{
    mLSFRData = 0xE1;  // Must be non-zero (or we get stuck)
}

RFFESimulationDataGenerator::~RFFESimulationDataGenerator()
{
}

void RFFESimulationDataGenerator::Initialize( U32 simulation_sample_rate,
                                              RFFEAnalyzerSettings* settings )
{
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings = settings;

    mClockGenerator.Init( simulation_sample_rate / 10, simulation_sample_rate );

    if( settings->mSclkChannel != UNDEFINED_CHANNEL )
        mSclk = mRffeSimulationChannels.Add( settings->mSclkChannel,
                                              mSimulationSampleRateHz,
                                              BIT_LOW );
    else
        mSclk = NULL;

    if( settings->mSdataChannel != UNDEFINED_CHANNEL )
        mSdata = mRffeSimulationChannels.Add( settings->mSdataChannel,
                                              mSimulationSampleRateHz,
                                              BIT_LOW );
    else
        mSdata = NULL;

    //insert 10 bit-periods of idle
    mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) );

    mParityCounter = 0;
}

U32 RFFESimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested,
                                                         U32 sample_rate,
                                                         SimulationChannelDescriptor** simulation_channels )
{
    U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested,
                                                                                           sample_rate,
                                                                                           mSimulationSampleRateHz );

    while( mSclk->GetCurrentSampleNumber() < adjusted_largest_sample_requested )
    {
        CreateRffeTransaction();

        mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 80.0 ) );
    }

    *simulation_channels = mRffeSimulationChannels.GetArray();
    return mRffeSimulationChannels.GetCount();
}

void RFFESimulationDataGenerator::CreateRffeTransaction()
{
    U8 cmd;
    U8 sa_addrs[] = { 0x5 };

    for( U32 adr=0 ; adr < sizeof(sa_addrs)/sizeof(sa_addrs[0]) ; adr++ )
    {
        for (U32 i=0; i < 256; i += 1)
        {
            CreateStart();
            CreateSlaveAddress( sa_addrs[adr] );
            cmd = i & 0xff;
            CreateCommandFrame( cmd );

            switch ( RFFEUtil::decodeRFFECmdFrame( cmd ) )
            {
            case RFFEAnalyzerResults::RffeTypeExtWrite:
                CreateAddressFrame( CreateRandomData() );
                for(U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
                    CreateDataFrame( CreateRandomData() );
                }
                CreateBusPark();
                break;
            case RFFEAnalyzerResults::RffeTypeReserved:
                CreateBusPark();
                break;
            case RFFEAnalyzerResults::RffeTypeExtRead:
                CreateAddressFrame( CreateRandomData() );
                CreateBusPark();
                for(U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
                    CreateDataFrame( CreateRandomData() );
                }
                CreateBusPark();
                break;
            case RFFEAnalyzerResults::RffeTypeExtLongWrite:
                CreateAddressFrame( CreateRandomData() );
                CreateAddressFrame( CreateRandomData() );
                for(U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
                    CreateDataFrame( CreateRandomData() );
                }
                CreateBusPark();
                break;
            case RFFEAnalyzerResults::RffeTypeExtLongRead:
                CreateAddressFrame( CreateRandomData() );
                CreateAddressFrame( CreateRandomData() );
                CreateBusPark();
                for(U32 i = 0; i <= RFFEUtil::byteCount(cmd); i += 1) {
                    CreateDataFrame( CreateRandomData() );
                }
                CreateBusPark();
                break;
            case RFFEAnalyzerResults::RffeTypeNormalWrite:
                CreateDataFrame( CreateRandomData() );
                CreateBusPark();
                break;
            case RFFEAnalyzerResults::RffeTypeNormalRead:
                CreateBusPark();
                CreateDataFrame( CreateRandomData() );
                CreateBusPark();
                break;
            case RFFEAnalyzerResults::RffeTypeWrite0:
                CreateBusPark();
                break;
            }
        }
    }
}

void RFFESimulationDataGenerator::CreateStart()
{
    if ( mSclk->GetCurrentBitState() == BIT_HIGH )
    {
        mSclk->Transition();
    }

    if( mSdata->GetCurrentBitState() == BIT_HIGH )
    {
        mSdata->Transition();
    }

    mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 2.0 ) );

    // sdata pulse for 1-clock cycle
    mSdata->Transition();
    mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 2.0 ) );
    mSdata->Transition();
    // sdata and sclk state low for 1-clock cycle
    mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 2.0 ) );

    mParityCounter = 0;
}

void RFFESimulationDataGenerator::CreateSlaveAddress(U8 addr )
{
    U8 address = addr & 0x0F;
    BitExtractor adr_bits( address, AnalyzerEnums::MsbFirst, 4 );

    for( U32 i=0; i< 4; i++ )
    {
        mSclk->Transition();
        mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( .5 ) );

        mSdata->TransitionIfNeeded( adr_bits.GetNextBit() );

        if( mSdata->GetCurrentBitState() == BIT_HIGH )
            mParityCounter++;

        mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
        mSclk->Transition();

        mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
    }
}

void RFFESimulationDataGenerator::CreateByte(U8 cmd)
{
    BitState     bit;
    BitExtractor cmd_bits( cmd, AnalyzerEnums::MsbFirst, 8 );

    for( U32 i=0; i< 8; i++ )
    {
        mSclk->Transition();
        mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( .5 ) );

        bit = cmd_bits.GetNextBit();
        mSdata->TransitionIfNeeded( bit );

        if( bit == BIT_HIGH )
            mParityCounter++;

        mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
        mSclk->Transition();

        mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
    }
}

void RFFESimulationDataGenerator::CreateParity()
{
    mSclk->Transition();
    mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( .5 ) );

    if( AnalyzerHelpers::IsEven(mParityCounter) )
    {
        mSdata->TransitionIfNeeded( BIT_HIGH );
    }
    else
    {
        mSdata->TransitionIfNeeded( BIT_LOW );
    }

    mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
    mSclk->Transition();

    mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
}

void RFFESimulationDataGenerator::CreateBusPark()
{
    mSclk->Transition();
    mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( .5 ) );

    mSdata->TransitionIfNeeded( BIT_LOW );

    mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
    mSclk->Transition();

    mRffeSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
}

void RFFESimulationDataGenerator::CreateCommandFrame( U8 cmd )
{
    mParityCounter = 0;
    CreateByte( cmd );
    CreateParity();
}

void RFFESimulationDataGenerator::CreateAddressFrame( U8 addr )
{
    mParityCounter = 0;
    CreateByte( addr );
    CreateParity();
}

void RFFESimulationDataGenerator::CreateDataFrame( U8 data )
{
    mParityCounter = 0;
    CreateByte( data );
    CreateParity();
}

U8 RFFESimulationDataGenerator::CreateRandomData()
{
    U8 lsb;
    
    lsb = mLSFRData & 0x1;
    mLSFRData >>= 1;
    if (lsb) {
        mLSFRData ^= 0xB8;  // Maximum period polynomial for Galois LFSR of 8 bits
    }
    return mLSFRData;
}

