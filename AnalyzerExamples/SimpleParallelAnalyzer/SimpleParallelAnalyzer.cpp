#include "SimpleParallelAnalyzer.h"
#include "SimpleParallelAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

SimpleParallelAnalyzer::SimpleParallelAnalyzer()
:	Analyzer2(),  
	mSettings( new SimpleParallelAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

SimpleParallelAnalyzer::~SimpleParallelAnalyzer()
{
	KillThread();
}

void SimpleParallelAnalyzer::SetupResults()
{
	mResults.reset( new SimpleParallelAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
}

void SimpleParallelAnalyzer::WorkerThread()
{
	mResults->AddChannelBubblesWillAppearOn( mSettings->mClockChannel );

	mSampleRateHz = GetSampleRate();

	AnalyzerResults::MarkerType clock_arrow;
	if( mSettings->mClockEdge == AnalyzerEnums::NegEdge )
		clock_arrow = AnalyzerResults::DownArrow;
	else
		clock_arrow = AnalyzerResults::UpArrow;


	mClock = GetAnalyzerChannelData( mSettings->mClockChannel );
	mData.clear();
	mDataMasks.clear();

	U32 count = mSettings->mDataChannels.size();
	for( U32 i=0; i<count; i++ )
	{
		if( mSettings->mDataChannels[i] != UNDEFINED_CHANNEL )
		{
			mData.push_back( GetAnalyzerChannelData( mSettings->mDataChannels[i] ) );
			mDataMasks.push_back( 1 << i );
			mDataChannels.push_back( mSettings->mDataChannels[i] );
		}
	}


	U32 num_data_lines = mData.size();

	if( mSettings->mClockEdge == AnalyzerEnums::NegEdge )
	{
		if( mClock->GetBitState() == BIT_LOW )
			mClock->AdvanceToNextEdge();
	}else
	{
		if( mClock->GetBitState() == BIT_HIGH )
			mClock->AdvanceToNextEdge();
	}


	mClock->AdvanceToNextEdge();  //this is the data-valid edge

	for( ; ; )
	{
		
		
		U64 sample = mClock->GetSampleNumber();
		mResults->AddMarker( sample, clock_arrow, mSettings->mClockChannel );

		U16 result = 0;

		for( U32 i=0; i<num_data_lines; i++ )
		{
			mData[i]->AdvanceToAbsPosition( sample );
			if( mData[i]->GetBitState() == BIT_HIGH )
			{
				result |= mDataMasks[i];
			}
			mResults->AddMarker( sample, AnalyzerResults::Dot, mDataChannels[i] );
		}	

		mClock->AdvanceToNextEdge(); 
		mClock->AdvanceToNextEdge();  //this is the data-valid edge

		//we have a byte to save. 
		Frame frame;
		frame.mData1 = result;
		frame.mFlags = 0;
		frame.mStartingSampleInclusive = sample;
		frame.mEndingSampleInclusive = mClock->GetSampleNumber() - 1;

		mResults->AddFrame( frame );
		mResults->CommitResults();
		ReportProgress( frame.mEndingSampleInclusive );
	}
}

bool SimpleParallelAnalyzer::NeedsRerun()
{
	return false;
}

U32 SimpleParallelAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 SimpleParallelAnalyzer::GetMinimumSampleRateHz()
{
	return 1000000;
}

const char* SimpleParallelAnalyzer::GetAnalyzerName() const
{
	return "Simple Parallel";
}

const char* GetAnalyzerName()
{
	return "Simple Parallel";
}

Analyzer* CreateAnalyzer()
{
	return new SimpleParallelAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
