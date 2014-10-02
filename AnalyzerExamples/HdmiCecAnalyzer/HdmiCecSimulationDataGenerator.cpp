#include "HdmiCecSimulationDataGenerator.h"
#include "HdmiCecAnalyzerSettings.h"

#include "HdmiCecProtocol.h"

#include <AnalyzerHelpers.h>

// Include for rand()
#include <stdlib.h>

HdmiCecSimulationDataGenerator::HdmiCecSimulationDataGenerator()
{
}

HdmiCecSimulationDataGenerator::~HdmiCecSimulationDataGenerator()
{
}

void HdmiCecSimulationDataGenerator::Initialize( U32 simulation_sample_rate, HdmiCecAnalyzerSettings* settings )
{
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings = settings;
    mSimulateErrors = false; // NOTICE change this to introduce simulation errors
    mErrorType = ERR_NOERROR;

    // Initialize clock at the recomended sampling rate
    mClockGenerator.Init( HdmiCec::MinSampleRateHz, mSimulationSampleRateHz );

    // Initialize the random number generator with a literal seed to obtain repeatability
    // Change this for srand(time(NULL)) for "truly" random sequences
    // NOTICE rand() an srand() are *not* thread safe
    srand( 42 );

    mCecSimulationData.SetChannel( mSettings->mCecChannel );
    mCecSimulationData.SetSampleRate( simulation_sample_rate );
    mCecSimulationData.SetInitialBitState( BIT_HIGH );
    // Advance a few ms in HIGH
    AdvanceRand( 0.5f, 2.0f );
}

U32 HdmiCecSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
    const U64 lastSample = AnalyzerHelpers::AdjustSimulationTargetSample(
                largest_sample_requested, sample_rate, mSimulationSampleRateHz );

    while( mCecSimulationData.GetCurrentSampleNumber() < lastSample )
    {
        SetRandomErrorType();
        GenVersionTransaction();
        AdvanceRand( 5.0f, 15.0f );

        SetRandomErrorType();
        GetStandbyTransaction();
        AdvanceRand( 5.0f, 15.0f );

        SetRandomErrorType();
        GetInitTransaction();
        AdvanceRand( 5.0f, 15.0f );
    }

    *simulation_channel = &mCecSimulationData;
    return 1; // We are simulating one channel
}

//
// Transaction generation
//

void HdmiCecSimulationDataGenerator::GenVersionTransaction()
{
    // The TV asks Tuner1 for it's CEC version
    GenStartSeq();
    GenHeaderBlock( HdmiCec::DevAddress_TV, HdmiCec::DevAddress_Tuner1 );
    AdvanceRand( 0.2f, 0.8f );
    if( mErrorType == ERR_WRONGEOM )
    {
        GenDataBlock( HdmiCec::OpCode_GetCecVersion, false, true );
        mErrorType = ERR_NOERROR;
    }
    else
    {
        GenDataBlock( HdmiCec::OpCode_GetCecVersion, true, true );
    }

    // Tuner1 asks with a CecVersion opcode and 0x4 (CEC 1.3a) as a single operand
    AdvanceRand( 5.0f, 10.0f );
    GenStartSeq();
    GenHeaderBlock( HdmiCec::DevAddress_Tuner1, HdmiCec::DevAddress_TV );
    AdvanceRand( 0.2f, 0.8f );
    GenDataBlock( HdmiCec::OpCode_CecVersion, false, true );
    AdvanceRand( 0.2f, 0.8f );
    GenDataBlock( 0x4, true, true );
}

void HdmiCecSimulationDataGenerator::GetStandbyTransaction()
{
    // Audio System sends the TV an standby command
    GenStartSeq();
    GenHeaderBlock( HdmiCec::DevAddress_AudioSystem, HdmiCec::DevAddress_TV );
    AdvanceRand( 0.2f, 0.8f );
    if( mErrorType == ERR_WRONGEOM )
    {
        GenDataBlock( HdmiCec::OpCode_Standby, false, true );
        mErrorType = ERR_NOERROR;
    }
    else
    {
        GenDataBlock( HdmiCec::OpCode_Standby, true, true );
    }

    // The TV decides to forward this message to all the devices
    AdvanceRand( 5.0f, 10.0f );
    GenStartSeq();
    GenHeaderBlock( HdmiCec::DevAddress_TV, HdmiCec::DevAddress_UnregBcast );
    AdvanceRand( 0.2f, 0.8f );
    GenDataBlock( HdmiCec::OpCode_Standby, true, true );
}

void HdmiCecSimulationDataGenerator::GetInitTransaction()
{
    // Tuner1 sends a header block to it's same address to verify that no one
    // ACKs the block (ie. the logical address is available)
    GenStartSeq();
    GenHeaderBlock( HdmiCec::DevAddress_Tuner1, HdmiCec::DevAddress_Tuner1, true, false );

    // Tuner1 reports physical address to bcast
    AdvanceRand( 5.0f, 10.0f );
    GenStartSeq();
    GenHeaderBlock( HdmiCec::DevAddress_Tuner1, HdmiCec::DevAddress_UnregBcast, false, false );
    AdvanceRand( 0.2f, 0.8f );
    GenDataBlock( HdmiCec::OpCode_ReportPhysicalAddress, false, false );
    AdvanceRand( 0.2f, 0.8f );
    GenDataBlock( 0x10, false, false );
    if( mErrorType == ERR_WRONGEOM )
    {
        GenDataBlock( 0x00, true, false );
        mErrorType = ERR_NOERROR;
    }
    else
    {
        GenDataBlock( 0x00, false, false );
    }
    GenDataBlock( 0x03, true, false );
}


//
// Start sequence, block and bit generation
//
void HdmiCecSimulationDataGenerator::GenStartSeq()
{
    if( mErrorType == ERR_NOSTARTSEQ )
    {
        mErrorType = ERR_NOERROR;
        return;
    }

    // The bus must be in high
    mCecSimulationData.TransitionIfNeeded( BIT_HIGH );

    // Timing values from CEC 1.3a section 5.2.1 "Start Bit Timing"
    mCecSimulationData.Transition(); // HIGH to LOW
    Advance( HdmiCec::Tim_Start_A );

    mCecSimulationData.Transition(); // LOW to HIGH
    Advance( HdmiCec::Tim_Start_B - HdmiCec::Tim_Start_A );
}

void HdmiCecSimulationDataGenerator::GenHeaderBlock( U8 src, U8 dst, bool eom, bool ack )
{
    const U8 data = ( ( src & 0xF ) << 4) | ( dst & 0xF );
    GenDataBlock( data, eom, ack );
}

void HdmiCecSimulationDataGenerator::GenDataBlock( U8 data, bool eom, bool ack )
{
    for( int i=7; i>=0; i-- )
        GenBit( (data >> i) & 0x1 );

    if( mErrorType != ERR_NOEOM )
        GenBit( eom );
    else
        mErrorType = ERR_NOERROR;

    if( mErrorType != ERR_NOACK )
        GenBit( ack, true );
    else
        mErrorType = ERR_NOERROR;
}

void HdmiCecSimulationDataGenerator::GenBit( bool value, bool ackBit )
{
    // Timing values are inverted for the follower-asserted ACK bit
    if( ackBit ) value = !value;
    // Timing values from CEC 1.3a section 5.2.2 "Data Bit Timing"
    const float risingTime = value ? HdmiCec::Tim_Bit_One : HdmiCec::Tim_Bit_Zero;

    // We should be in LOW
    mCecSimulationData.TransitionIfNeeded( BIT_LOW );

    Advance( risingTime );
    mCecSimulationData.Transition(); // LOW to HIGH

    Advance( HdmiCec::Tim_Bit_Len - risingTime );
}


void HdmiCecSimulationDataGenerator::Advance( float msecs )
{
    mCecSimulationData.Advance( mClockGenerator.AdvanceByTimeS( msecs / 1000.0 ) );
}

void HdmiCecSimulationDataGenerator::AdvanceRand( float minMsecs, float maxMsecs )
{
    // Get a random number from 0 to 1
    float r = frand();
    // Use r in a weighted sum to obtain a random number from minMsecs to maxMsecs
    Advance( (1.0f-r) * minMsecs + r * maxMsecs );
}

float HdmiCecSimulationDataGenerator::frand()
{
    return static_cast<float>( rand() ) / RAND_MAX;
}

void HdmiCecSimulationDataGenerator::SetRandomErrorType()
{
    if( mSimulateErrors )
        mErrorType = static_cast<ErrorType>( rand() % 5 );
    else
        mErrorType = ERR_NOERROR;
}
