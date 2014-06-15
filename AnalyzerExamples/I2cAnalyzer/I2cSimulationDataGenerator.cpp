#include "I2cSimulationDataGenerator.h"


I2cSimulationDataGenerator::I2cSimulationDataGenerator()
{
}

I2cSimulationDataGenerator::~I2cSimulationDataGenerator()
{
}

void I2cSimulationDataGenerator::Initialize( U32 simulation_sample_rate, I2cAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mClockGenerator.Init( 400000, simulation_sample_rate );

	mSda = mI2cSimulationChannels.Add( settings->mSdaChannel, mSimulationSampleRateHz, BIT_HIGH );
	mScl = mI2cSimulationChannels.Add( settings->mSclChannel, mSimulationSampleRateHz, BIT_HIGH );

	mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 10.0 ) ); //insert 10 bit-periods of idle

	mValue = 0;
}

U32 I2cSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mScl->GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		CreateI2cTransaction( 0xA0, I2C_READ, mValue++ );

		mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 20.0 ) ); //insert 20 bit-periods of idle
	}

	*simulation_channels = mI2cSimulationChannels.GetArray();
	return mI2cSimulationChannels.GetCount();
}



void I2cSimulationDataGenerator::CreateI2cTransaction( U8 address, I2cDirection direction, U8 data )
{
	U8 command = address << 1;
	if( direction == I2C_READ )
		command |= 0x1;

	CreateStart();
	CreateI2cByte( command, I2C_ACK );
	CreateI2cByte( data, I2C_ACK );
	CreateI2cByte( data, I2C_NAK );
	CreateStop();
}

void I2cSimulationDataGenerator::CreateI2cByte( U8 data, I2cResponse reply )
{
	if( mScl->GetCurrentBitState() == BIT_HIGH )
	{
		mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
		mScl->Transition();
		mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
	}

	BitExtractor bit_extractor( data, AnalyzerEnums::MsbFirst, 8 );

	for( U32 i=0; i<8; i++ )
	{
		CreateBit( bit_extractor.GetNextBit() );
	}

	if( reply == I2C_ACK )
		CreateBit( BIT_LOW );
	else
		CreateBit( BIT_HIGH );

	mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 4.0 ) );
}

void I2cSimulationDataGenerator::CreateBit( BitState bit_state )
{
	if( mScl->GetCurrentBitState() != BIT_LOW )
		AnalyzerHelpers::Assert( "CreateBit expects to be entered with scl low" );

	mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );

	mSda->TransitionIfNeeded( bit_state );

	mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 0.5 ) );

	mScl->Transition(); //posedge

	mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );

	mScl->Transition(); //negedge
}

void I2cSimulationDataGenerator::CreateStart()
{
	mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );

	//1st, we need to make SDA high, 
	SafeChangeSda( BIT_HIGH );

	//2nd, we need make the clock high.
	if( mScl->GetCurrentBitState() == BIT_LOW )
	{
		mScl->Transition();
		mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
	}

	//3rd, bring SDA high.
	mSda->Transition();
	mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
}


void I2cSimulationDataGenerator::CreateStop()
{
	mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );

	//1st, we need to make SDA low, 
	SafeChangeSda( BIT_LOW );

	//2nd, we need make the clock high.
	if( mScl->GetCurrentBitState() == BIT_LOW )
	{
		mScl->Transition();
		mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
	}

	//3rd, bring SDA high.
	mSda->Transition();
	mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
}

void I2cSimulationDataGenerator::CreateRestart()
{
	CreateStart();
}

void I2cSimulationDataGenerator::SafeChangeSda( BitState bit_state )
{
	if( mSda->GetCurrentBitState() != bit_state )
	{
		//make sure SCK is low before we toggle it
		if( mScl->GetCurrentBitState() == BIT_HIGH )
		{
			mScl->Transition();
			mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
		}

		mSda->Transition();
		mI2cSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod( 1.0 ) );
	}	
}
