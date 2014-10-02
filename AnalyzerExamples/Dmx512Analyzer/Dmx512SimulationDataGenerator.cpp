#include "Dmx512SimulationDataGenerator.h"
#include "Dmx512AnalyzerSettings.h"
#include <AnalyzerHelpers.h>

Dmx512SimulationDataGenerator::Dmx512SimulationDataGenerator()
{
}

Dmx512SimulationDataGenerator::~Dmx512SimulationDataGenerator()
{
}

void Dmx512SimulationDataGenerator::Initialize( U32 simulation_sample_rate, Dmx512AnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;
	mClockGenerator.Init(mSettings->mBitRate, simulation_sample_rate);
	
	mSerialSimulationData.SetChannel( mSettings->mInputChannel );
	mSerialSimulationData.SetSampleRate( simulation_sample_rate );
	// In the absence of a valid DMX packet the output of a DMX
	// line will be a continously HI signal.
	mSerialSimulationData.SetInitialBitState( BIT_HIGH );
	mSlots = 257;
}


#ifdef EXTREME_VALUES
// [88us, 1s]
U32 Dmx512SimulationDataGenerator::BreakLength( void )
{
	static int i=0;
	if( i++ & 1 )
		return mClockGenerator.AdvanceByHalfPeriod( 22 );
	else
		return mClockGenerator.AdvanceByHalfPeriod( 250000 );
}

// [8us (or 4us), 1s]
U32 Dmx512SimulationDataGenerator::MABLength( void )
{
  static int i=0;
  if( i++ & 2 )
	  return mClockGenerator.AdvanceByHalfPeriod( 1 );
  else
	  return mClockGenerator.AdvanceByHalfPeriod( 250000 );
}

// [0, 1s]
U32 Dmx512SimulationDataGenerator::MarkLength( void )
{
  static int i=0;
  if( i++ & 4 )
	  return 0;
  else
	  return mClockGenerator.AdvanceByHalfPeriod( 250000 );
}

// MBB is actually MarkLength + MBBLength
// (0, 1s]
U32 Dmx512SimulationDataGenerator::MBBLength( void )
{
	return; // We just use MarkLength
}
#else // typical values
// [88us, 1s]
U32 Dmx512SimulationDataGenerator::BreakLength( void )
{
	return mClockGenerator.AdvanceByHalfPeriod( 25 );
}

// [8us (or 4us), 1s]
U32 Dmx512SimulationDataGenerator::MABLength( void )
{
	//	return mClockGenerator.AdvanceByHalfPeriod( 1 );
	return mClockGenerator.AdvanceByHalfPeriod( 3 );
}

// [0, 1s]
U32 Dmx512SimulationDataGenerator::MarkLength( void )
{
	return mClockGenerator.AdvanceByHalfPeriod( 2 );
}

// MBB is actually MarkLength + MBBLength
// (0, 1s]
U32 Dmx512SimulationDataGenerator::MBBLength( void )
{
	return mClockGenerator.AdvanceByHalfPeriod( 2 );
}
#endif

U32 Dmx512SimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );
	//U32 samples_per_bit = mSimulationSampleRateHz / mSettings->mBitRate;
	
	while( mSerialSimulationData.GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
	    // BREAK: hold LOW for 22-250,000 bits (88us-1s)
	    mSerialSimulationData.Transition();
	    mSerialSimulationData.Advance( BreakLength() );

		// MARK AFTER BREAK (MAB): go back high, max 1s
		// minimum 4us for DMX 1986, 8us for DMX-1990
	    mSerialSimulationData.Transition();
	    mSerialSimulationData.Advance( MABLength() );
		
	    // START CODE (SC): one bit low, then 8 bits data, then two bits HIGH
	    mSerialSimulationData.Transition();
	    mSerialSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 1 ) );
	    // first 8-bit code is always 0
	    mSerialSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 8 ) );
	    // 2 stop bits HIGH
	    mSerialSimulationData.Transition();
	    mSerialSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 2 ) );

	    // Mark after slot
	    mSerialSimulationData.Advance( MarkLength() );
	    
	    for (int j=0; j < mSlots; ++j)
		{
			// first bit is LOW
			mSerialSimulationData.Transition();
			mSerialSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 1 ) );
			// Send arbitary data (HIGH / LOW alternating)
			mSerialSimulationData.Transition();
			for (int i=0; i < 8; ++i)
			{
				mSerialSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 1 ) );
				mSerialSimulationData.Transition();
			}
		// 2 stop bits HIGH
		mSerialSimulationData.TransitionIfNeeded( BIT_HIGH );
		mSerialSimulationData.Advance( mClockGenerator.AdvanceByHalfPeriod( 2 ) );

		// MARK Time Between Frames (= Slots)
		// can range from 0ms to 1s
		mSerialSimulationData.Advance( MarkLength() );
	    }
	    // Simulate different-sized packets
	    mSlots = (mSlots + 257) % 513;

	    // MARK Before Break (MBB) aka MARK Time Between Packets, >0-1000ms
	    // (HIGH) can range from 0ms to 1s
	    mSerialSimulationData.Advance( MBBLength() );
	}

	*simulation_channel = &mSerialSimulationData;
	return 1;
}
