#include "ManchesterSimulationDataGenerator.h"
#include "ManchesterAnalyzerSettings.h"

ManchesterSimulationDataGenerator::ManchesterSimulationDataGenerator()
{

}

ManchesterSimulationDataGenerator::~ManchesterSimulationDataGenerator()
{

}

void ManchesterSimulationDataGenerator::Initialize( U32 simulation_sample_rate, ManchesterAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mManchesterSimulationData.SetChannel( mSettings->mInputChannel );
	mManchesterSimulationData.SetSampleRate( simulation_sample_rate );

	mManchesterSimulationData.SetInitialBitState( BIT_LOW );

	double half_period = 1.0 / double( mSettings->mBitRate * 2 );
	half_period *= 1000000.0;
	mT = UsToSamples( half_period );
	mSimValue = 1;

	if ( mSettings->mBitsPerTransfer > 32 )
	{
		mSimValue = 0xFFFFFFFF;

	}

	mManchesterSimulationData.Advance( U32(mT * 8) );
}

U32 ManchesterSimulationDataGenerator::GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( newest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mManchesterSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		for( U32 i = 0; i < mSettings->mBitsToIgnore; ++i )
			SimWriteBit( 0 );
		SimWriteByte( mSimValue++ );
		SimWriteByte( mSimValue++ );
		SimWriteByte( mSimValue++ );
		mManchesterSimulationData.Advance( U32(mT * 8) );
	}
	*simulation_channels = &mManchesterSimulationData;
	return 1;
}

U64 ManchesterSimulationDataGenerator::UsToSamples( U64 us )
{
	return ( mSimulationSampleRateHz * us ) / 1000000;
}

U64 ManchesterSimulationDataGenerator::UsToSamples( double us )
{
	return U64(( mSimulationSampleRateHz * us ) / 1000000.0);
}

U64 ManchesterSimulationDataGenerator::SamplesToUs( U64 samples )
{
	return( samples * 1000000 ) / mSimulationSampleRateHz;
}

void ManchesterSimulationDataGenerator::SimWriteByte( U64 value )
{
	U32 bits_per_xfer = mSettings->mBitsPerTransfer;

	for( U32 i = 0; i < bits_per_xfer; ++i )
	{
		if( mSettings->mShiftOrder == AnalyzerEnums::LsbFirst )
		{
			SimWriteBit( ( value >> i ) & 0x1 );
		}
		else if( mSettings->mShiftOrder == AnalyzerEnums::MsbFirst )
		{
			SimWriteBit( ( value >> (bits_per_xfer - i - 1) ) & 0x1 );
		}
	}
}

void ManchesterSimulationDataGenerator::SimWriteBit( U32 bit )
{
	BitState start_bit_state = mManchesterSimulationData.GetCurrentBitState();
	switch( mSettings->mMode )
	{
	case MANCHESTER:
		{
			if( mSettings->mInverted == false )
			{
				if( ( bit == 0 ) && ( start_bit_state == BIT_HIGH ) )
					mManchesterSimulationData.Transition();
				else if( ( bit == 1 ) && ( start_bit_state == BIT_LOW ) )
					mManchesterSimulationData.Transition(); 
			}
			else
			{
				if( ( bit == 1 ) && ( start_bit_state == BIT_HIGH ) )
					mManchesterSimulationData.Transition();
				else if( ( bit == 0 ) && ( start_bit_state == BIT_LOW ) )
					mManchesterSimulationData.Transition(); 
			}
				mManchesterSimulationData.Advance( U32(mT) );
				mManchesterSimulationData.Transition();
				mManchesterSimulationData.Advance( U32(mT) );
		}
		break;
	case DIFFERENTIAL_MANCHESTER:
		{
			if( bit == 0 )
				mManchesterSimulationData.Transition();
			mManchesterSimulationData.Advance( U32(mT) );
			mManchesterSimulationData.Transition();
			mManchesterSimulationData.Advance( U32(mT) );
		}
		break;
	case BI_PHASE_MARK:
		{
			mManchesterSimulationData.Transition();
			mManchesterSimulationData.Advance( U32(mT) );
			if( bit == 1 )
				mManchesterSimulationData.Transition();
			mManchesterSimulationData.Advance( U32(mT) );
		}
		break;
	case BI_PHASE_SPACE:
		{
			mManchesterSimulationData.Transition();
			mManchesterSimulationData.Advance( U32(mT) );
			if( bit == 0 )
				mManchesterSimulationData.Transition();
			mManchesterSimulationData.Advance( U32(mT) );
		}
		break;
	}
}
