#include "SimpleParallelSimulationDataGenerator.h"
#include "SimpleParallelAnalyzerSettings.h"

#include <AnalyzerHelpers.h>

SimpleParallelSimulationDataGenerator::SimpleParallelSimulationDataGenerator()
{
}

SimpleParallelSimulationDataGenerator::~SimpleParallelSimulationDataGenerator()
{
}

void SimpleParallelSimulationDataGenerator::Initialize( U32 simulation_sample_rate, SimpleParallelAnalyzerSettings* settings )
{
	mSettings = settings;
	mSimulationSampleRateHz = simulation_sample_rate;

	mData.clear();
	mDataMasks.clear();

	U32 count = mSettings->mDataChannels.size();
	for( U32 i=0; i<count; i++ )
	{
		mData.push_back( mSimulationData.Add( mSettings->mDataChannels[i], mSimulationSampleRateHz, BIT_LOW ) );
		U16 val = 1 << i;
		mDataMasks.push_back( val );
	}

	if( mSettings->mClockEdge == AnalyzerEnums::NegEdge )
		mClock = mSimulationData.Add( mSettings->mClockChannel, mSimulationSampleRateHz, BIT_LOW );
	else
		mClock = mSimulationData.Add( mSettings->mClockChannel, mSimulationSampleRateHz, BIT_HIGH );

	mValue = 0;
}

U32 SimpleParallelSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mClock->GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
		mSimulationData.AdvanceAll( 1000 );
		U32 count = mData.size();
		for( U32 i=0; i<count; i++ )
		{
			if( ( mDataMasks[i] & mValue ) == 0 )
				mData[i]->TransitionIfNeeded( BIT_LOW );
			else
				mData[i]->TransitionIfNeeded( BIT_HIGH );

		}
		mClock->Transition();

		mSimulationData.AdvanceAll( 1000 );
		mClock->Transition();

		mValue++;
	}

	*simulation_channel = mSimulationData.GetArray();
	return mSimulationData.GetCount();
}
