#pragma warning(push, 0)
#include <sstream>
#include <ios>
//#include <boost/archive/text_iarchive.hpp>
//#include <boost/thread.hpp>
#pragma warning(pop)

#include "OneWireAnalyzer.h"
#include "OneWireAnalyzerSettings.h"  
#include <AnalyzerChannelData.h>
//#include <LogicDebug.h>
//#include "AnalyzerHelpers.h"
//#include "ChannelData.h" 
//#include "ThreadPriority.h"

//#pragma warning(pop) //boost thread adds an extra push, unfortuantly!

OneWireAnalyzer::OneWireAnalyzer() 
:	mSettings( new OneWireAnalyzerSettings() ),
	Analyzer2(),
	mSimulationInitilized( false )
{ 
	SetAnalyzerSettings( mSettings.get() );
}

OneWireAnalyzer::~OneWireAnalyzer()
{
	KillThread();
}

void OneWireAnalyzer::SetupResults()
{
	mResults.reset( new OneWireAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mOneWireChannel );
}

void OneWireAnalyzer::WorkerThread()
{
	mResults->AddChannelBubblesWillAppearOn( mSettings->mOneWireChannel );

	mOneWire = GetAnalyzerChannelData( mSettings->mOneWireChannel );

	U64 starting_sample  = mOneWire->GetSampleNumber();
	//one_wire.MoveToSample( starting_sample );

	U64 current_sample = starting_sample;

	mSampleRateHz = this->GetSampleRate();

	mCurrentState = UnknownState;
	mOverdrive = false;
	
	mRisingEdgeSample = current_sample;
	mFallingEdgeSample = 0;
	mByteStartSample = 0;
	mBlockPulseAdvance = false;

	for( ; ; )
	{
		if( mBlockPulseAdvance == false )
		{
			mPreviousRisingEdgeSample = mRisingEdgeSample;

			//one_wire.MoveRightUntilBitChanges( true, true );
			//most of the time, we will be entering on a falling edge.

			current_sample = mOneWire->GetSampleNumber();
			if( (mOneWire->GetBitState() == BIT_LOW) &&(current_sample > mFallingEdgeSample) )
			{
				//it is possible that we have advanced into a reset pulse.
				//mFallingEdgeSample will contain the falling edge that we came in on. we should be inside by either 3us or 30 us.
				
				U64 next_rising_edge_sample = mOneWire->GetSampleOfNextEdge();
				U64 low_pulse_length = next_rising_edge_sample - mFallingEdgeSample;

				//test for a reset condition.
				U64 minimum_pulse_width = UsToSamples( SPEC_RESET_PULSE - MARGIN_INSIDE_RESET_PULSE );
				U64 spec_pulse_width = UsToSamples( SPEC_RESET_PULSE );
				if( mOverdrive == true )
				{
					minimum_pulse_width = UsToSamples( SPEC_OVD_RESET_PULSE - MARGIN_INSIDE_OVD_RESET_PULSE );
					spec_pulse_width = UsToSamples( SPEC_OVD_RESET_PULSE );
				}
				if ( low_pulse_length > minimum_pulse_width )
				{
					//Reset Pulse Detected.
					//Print Reset Bubble.
					bool flag_warning = false;
					if( mOverdrive == true )
					{
						if( low_pulse_length > UsToSamples( SPEC_RESET_PULSE - MARGIN_INSIDE_RESET_PULSE ) )
						{ 
							if( low_pulse_length < UsToSamples( SPEC_RESET_PULSE ) )
								flag_warning = true;
						}
					}
					else
					{
						if( low_pulse_length < spec_pulse_width )
							flag_warning = true;
					}
					mResults->CommitPacketAndStartNewPacket();
					if( flag_warning == true )
						//TODO:  RecordBubble( one_wire_bubbles, mFallingEdgeSample, mRisingEdgeSample, RestartPulse, mLowPulseLength );
						RecordFrame(mFallingEdgeSample, next_rising_edge_sample, RestartPulse, 0, true );
					else
						//TODO:  RecordBubble( one_wire_bubbles, mFallingEdgeSample, mRisingEdgeSample, RestartPulse );
						RecordFrame(mFallingEdgeSample, next_rising_edge_sample, RestartPulse );

					if (low_pulse_length > UsToSamples( SPEC_RESET_PULSE - MARGIN_INSIDE_RESET_PULSE ))
						mOverdrive = false;

					mRomBitsRecieved = 0;
					mRomDetected = 0;
					mDataDetected = 0;
					mDataBitsRecieved = 0;

					mCurrentState = ResetDetectedState;
					mOneWire->AdvanceToNextEdge(); //advance to the rising edge out of the reset pulse, so we don't detect it twice.
					
					continue;
				}
			} 
			
			mOneWire->AdvanceToNextEdge();
			
			//if( one_wire.GetBitState() != BIT_LOW )
			//	one_wire.MoveRightUntilBitChanges( true, true );

			//this happens every time a zero is recorded.
			if( mOneWire->GetBitState() != BIT_LOW )
				mOneWire->AdvanceToNextEdge();

			mFallingEdgeSample = mOneWire->GetSampleNumber();//one_wire.GetSampleNumber();

			mHighPulseLength = mFallingEdgeSample - mRisingEdgeSample;

			//one_wire.MoveRightUntilBitChanges( true, true );
			mRisingEdgeSample = mOneWire->GetSampleOfNextEdge();
			//mOneWire->AdvanceToNextEdge();

			//mRisingEdgeSample = mOneWire->GetSampleNumber();//one_wire.GetSampleNumber();
			mLowPulseLength = mRisingEdgeSample - mFallingEdgeSample;
		
			mLowPulseTime = SamplesToUs( mLowPulseLength ); //micro seconds
			mHighPulseTime = SamplesToUs( mHighPulseLength );

			U64 min_low_pulse_samples = UsToSamples( 1 );
			if( (mOverdrive == true) && (mSampleRateHz <= 1000000 ) )
				min_low_pulse_samples = 0;

			while( mLowPulseLength < min_low_pulse_samples )
			{
				mOneWire->AdvanceToNextEdge();
				mOneWire->AdvanceToNextEdge(); //next neg edge.
				mFallingEdgeSample = mOneWire->GetSampleNumber();//one_wire.GetSampleNumber();
				mRisingEdgeSample = mOneWire->GetSampleOfNextEdge();
				mLowPulseLength = mRisingEdgeSample - mFallingEdgeSample;
				mLowPulseTime = SamplesToUs( mLowPulseLength ); //micro seconds
			}
			

			//At this point, we should be sitting on a negative edge, and have the pulse length of the low pulse in front, and the high pulse behind.
		}
		mBlockPulseAdvance = false;

		//Test for a reset Pulse:
		{
			U64 minimum_pulse_width = UsToSamples( SPEC_RESET_PULSE - MARGIN_INSIDE_RESET_PULSE );
			U64 spec_pulse_width = UsToSamples( SPEC_RESET_PULSE );
			if( mOverdrive == true )
			{
				minimum_pulse_width = UsToSamples( SPEC_OVD_RESET_PULSE - MARGIN_INSIDE_OVD_RESET_PULSE );
				spec_pulse_width = UsToSamples( SPEC_OVD_RESET_PULSE );
			}
			if ( mLowPulseLength > minimum_pulse_width )
			{
				//Reset Pulse Detected.
				//Print Reset Bubble.
				bool flag_warning = false;
				if( mOverdrive == true )
				{
					if( mLowPulseLength > UsToSamples( SPEC_RESET_PULSE - MARGIN_INSIDE_RESET_PULSE ) )
					{ 
						if( mLowPulseLength < UsToSamples( SPEC_RESET_PULSE ) )
							flag_warning = true;
					}
				}
				else
				{
					if( mLowPulseLength < spec_pulse_width )
						flag_warning = true;
				}
				mResults->CommitPacketAndStartNewPacket();
				if( flag_warning == true )
					//TODO:  RecordBubble( one_wire_bubbles, mFallingEdgeSample, mRisingEdgeSample, RestartPulse, mLowPulseLength );
					RecordFrame(mFallingEdgeSample, mRisingEdgeSample, RestartPulse, 0, true );
				else
					//TODO:  RecordBubble( one_wire_bubbles, mFallingEdgeSample, mRisingEdgeSample, RestartPulse );
					RecordFrame(mFallingEdgeSample, mRisingEdgeSample, RestartPulse );

				if (mLowPulseLength > UsToSamples( SPEC_RESET_PULSE - MARGIN_INSIDE_RESET_PULSE ))
					mOverdrive = false;

				mRomBitsRecieved = 0;
				mRomDetected = 0;
				mDataDetected = 0;
				mDataBitsRecieved = 0;

				mCurrentState = ResetDetectedState;
				continue;
			}
		}

		switch ( mCurrentState )
		{
		case UnknownState:
			{
				//Do nothing.
			}
			break;
		case ResetDetectedState:
			{
				//Test to see if er have a reset pulse.
				U64 minimum_pulse_width = UsToSamples( SPEC_MIN_PRESENCE_PULSE - MARGIN_INSIDE_PRESENCE_PULSE );
				
				if( mOverdrive == true )
				{
					minimum_pulse_width = UsToSamples( SPEC_MIN_OVD_PRESENCE_PULSE - MARGIN_INSIDE_OVD_PRESENCE_PULSE );
				}
				if ( mLowPulseLength > minimum_pulse_width )
				{
					//Presence Pulse Detected.
					//TODO:   RecordBubble( one_wire_bubbles, mFallingEdgeSample, mRisingEdgeSample, PresencePulse );
					RecordFrame( mFallingEdgeSample, mRisingEdgeSample, PresencePulse );

					mCurrentState = PresenceDetectedState;
				}
			}
			break;
		case PresenceDetectedState:
			{
				//anticipating Rom Command.
				if( mDataBitsRecieved == 0 )
					mByteStartSample = mFallingEdgeSample;
				/*
				U64 minimum_pulse_width_zero = UsToSamples( SPEC_MIN_ZERO_PULSE - MARGIN_INSIDE_ZERO_PULSE ); //60us spec
				U64 maximum_pulse_width_one = UsToSamples( SPEC_MAX_ONE_PULSE + MARGIN_OUTSIDE_ONE_PULSE ); //15us spec
				if( mOverdrive == true )
				{
					minimum_pulse_width_zero = UsToSamples( SPEC_OVD_MIN_ZERO_PULSE - MARGIN_INSIDE_OVD_ZERO_PULSE ); //6us spec
					maximum_pulse_width_one = UsToSamples( SPEC_MAX_OVD_ONE_PULSE + MARGIN_OUTSIDE_OVD_ONE_PULSE ); //2us spec
				}
				if ( mLowPulseLength > minimum_pulse_width_zero )
				{
					//store a zero into the data.
				}
				else if( mLowPulseLength <= maximum_pulse_width_one )
				{
					//store a one into the data.
					if( ( mOverdrive == true ) || ( mLowPulseLength > 1 ) )
						mDataDetected |= U64(0x1) << mDataBitsRecieved;
				}
				else
				{
					mCurrentState = UnknownState;
					//Something Broke.
				}
				*/
				U64 sample_location_offset = UsToSamples( SPEC_SAMPLE_POINT );
				if( mOverdrive == true )
					sample_location_offset = UsToSamples( SPEC_OVD_SAMPLE_POINT );
				mOneWire->Advance( sample_location_offset );
				
				if( mOneWire->GetBitState() == BIT_HIGH )
				{
					//short pulse, this is a 1
					mDataDetected |= U64(0x1) << mDataBitsRecieved;
				}
				else
				{
					//long pulse, this is a 0
				}
				//if( ( mOverdrive == true ) || ( mLowPulseLength > 1 ) )
				mDataBitsRecieved += 1;

				if( mDataBitsRecieved == 8 )
				{
					//all 8 bits of rom command collected.
					//add bubble for all 8 bits.
					//ReadRom, SkipRom, SearchRom, MatchRom, OverdriveSkipRom, OverdriveMatchRom 
					OneWireFrameType frame_type;

					if( mDataDetected == 0x33 )
					{
						mCurrentRomCommand = ReadRom;
						frame_type = ReadRomFrame;
					}
					else if( mDataDetected == 0xCC )
					{
						mCurrentRomCommand = SkipRom;
						frame_type = SkipRomFrame;
					}
					else if( mDataDetected == 0x55 )
					{
						mCurrentRomCommand = MatchRom;
						frame_type = MatchRomFrame;
					}
					else if( mDataDetected == 0xF0 )
					{
						mCurrentRomCommand = SearchRom;
						frame_type = SearchRomFrame;
					}
					else if( mDataDetected == 0x3C )
					{
						mCurrentRomCommand = OverdriveSkipRom;
						frame_type = OverdriveSkipRomFrame;
					}
					else if( mDataDetected == 0x69 )
					{
						mCurrentRomCommand = OverdriveMatchRom;
						frame_type = OverdriveMatchRomFrame;
					}
					else
					{
						frame_type = InvalidRomCommandFrame;
						mCurrentState = RomFinishedState;
					}

					if(mCurrentState != RomFinishedState)
						mCurrentState = RomCommandDetectedState;


					//TODO:  RecordBubble( one_wire_bubbles, mByteStartSample, mRisingEdgeSample, RomCommand, mCurrentRomCommand, mDataDetected );
					RecordFrame( mByteStartSample, mRisingEdgeSample, frame_type, mDataDetected );
	

					mDataBitsRecieved = 0;
					mDataDetected = 0;		
					
					continue;
				}

			}
			break;
		case RomCommandDetectedState:
			{
				//ReadRom, SkipRom, SearchRom, MatchRom, OverdriveSkipRom, OverdriveMatchRom
				switch( mCurrentRomCommand )
				{
				case ReadRom:
					{
						//expecting 64 bits of rom address, nothing more.
						//mRomDetected
						if( mRomBitsRecieved == 8 )
						{
							//Bubble the ROM Family Code.
							//TODO: RecordBubble( one_wire_bubbles, mByteStartSample, mPreviousRisingEdgeSample, FamilyCode, mRomDetected );
							//RecordFrame( mByteStartSample, mPreviousRisingEdgeSample, FamilyCode, mRomDetected );
							mByteStartSample = mFallingEdgeSample;
						}
						if( mRomBitsRecieved == 56 )
						{
							//Bubble the ROM Data bits.
							//TODO:  RecordBubble( one_wire_bubbles, mByteStartSample, mPreviousRisingEdgeSample, Rom, ((mRomDetected >> 8) & 0xFFFFFFFFFFFFull) );
							//RecordFrame( mByteStartSample, mPreviousRisingEdgeSample, Rom, ((mRomDetected >> 8) & 0xFFFFFFFFFFFFull) );
							mByteStartSample = mFallingEdgeSample;
						}
						if( mRomBitsRecieved == 0 )
						{
							//Tag start of Family Code.
							mByteStartSample = mFallingEdgeSample;
						}
						/*
						U64 minimum_pulse_width_zero = UsToSamples( SPEC_MIN_ZERO_PULSE - MARGIN_INSIDE_ZERO_PULSE ); //60us spec
						U64 maximum_pulse_width_one = UsToSamples( SPEC_MAX_ONE_PULSE + MARGIN_OUTSIDE_ONE_PULSE ); //15us spec
						if( mOverdrive == true )
						{
							minimum_pulse_width_zero = UsToSamples( SPEC_OVD_MIN_ZERO_PULSE - MARGIN_INSIDE_OVD_ZERO_PULSE ); //6us spec
							maximum_pulse_width_one = UsToSamples( SPEC_MAX_OVD_ONE_PULSE + MARGIN_OUTSIDE_OVD_ONE_PULSE ); //2us spec
						}
						if ( mLowPulseLength > minimum_pulse_width_zero )
						{
							//store a zero into the data.
						}
						else if( mLowPulseLength <= maximum_pulse_width_one )
						{
							//store a one into the data.
							mRomDetected |= U64(0x1) << mRomBitsRecieved;
						}
						else
						{
							mCurrentState = UnknownState;
							//Something Broke.
						}
						*/
						U64 sample_location_offset = UsToSamples( SPEC_SAMPLE_POINT );
						if( mOverdrive == true )
							sample_location_offset = UsToSamples( SPEC_OVD_SAMPLE_POINT );
						mOneWire->Advance( sample_location_offset );
						
						if( mOneWire->GetBitState() == BIT_HIGH )
						{
							//short pulse, this is a 1
							mRomDetected |= U64(0x1) << mRomBitsRecieved;
						}
						else
						{
							//long pulse, this is a 0
						}
						U64 temp = SamplesToUs( mLowPulseLength );
						mRomBitsRecieved += 1;

						if( mRomBitsRecieved == 8 )
							RecordFrame( mByteStartSample, mRisingEdgeSample, FamilyCode, mRomDetected );


						if( mRomBitsRecieved == 56 )
							RecordFrame( mByteStartSample, mRisingEdgeSample, Rom, ((mRomDetected >> 8) & 0xFFFFFFFFFFFFull) );

						if( mRomBitsRecieved == 64 )
						{
							//Bubble the ROM CRC.
							//TODO:  RecordBubble( one_wire_bubbles, mByteStartSample, mRisingEdgeSample, CRC, (mRomDetected >> 56) );
							RecordFrame( mByteStartSample, mRisingEdgeSample, CRC, (mRomDetected >> 56) );
							//Move to the next state
							mCurrentState = RomFinishedState;

						}
					}
					break;
				case SkipRom:
					{
						mCurrentState = RomFinishedState;
						//we cannot through away our current transision!
						mBlockPulseAdvance = true;
					}
					break;
				case SearchRom:
					{
						//expecting 192 bits of crap :( perhaps we should filter out only the ROM select bits?
						//mRomDetected
						if( mRomBitsRecieved == 24 )
						{
							//Bubble the ROM Family Code.
							//TODO:  RecordBubble( one_wire_bubbles, mByteStartSample, mPreviousRisingEdgeSample, FamilyCode, mRomDetected );
							//RecordFrame( mByteStartSample, mPreviousRisingEdgeSample, FamilyCode, mRomDetected );
							mByteStartSample = mFallingEdgeSample;
						}
						if( mRomBitsRecieved == 168 )
						{
							//Bubble the ROM Data bits.
							//TODO:  RecordBubble( one_wire_bubbles, mByteStartSample, mPreviousRisingEdgeSample, Rom, ((mRomDetected >> 8) & 0xFFFFFFFFFFFFull) );
							//RecordFrame( mByteStartSample, mPreviousRisingEdgeSample, Rom, ((mRomDetected >> 8) & 0xFFFFFFFFFFFFull) );
							mByteStartSample = mFallingEdgeSample;
						}
						if( mRomBitsRecieved == 0 )
						{
							//Tag start of Family Code.
							mByteStartSample = mFallingEdgeSample;
						}
						if( ((mRomBitsRecieved + 1) % 3) == 0 )
						{
							/*
							U64 minimum_pulse_width_zero = UsToSamples( SPEC_MIN_ZERO_PULSE - MARGIN_INSIDE_ZERO_PULSE ); //60us spec
							U64 maximum_pulse_width_one = UsToSamples( SPEC_MAX_ONE_PULSE + MARGIN_OUTSIDE_ONE_PULSE ); //15us spec
							if( mOverdrive == true )
							{
								minimum_pulse_width_zero = UsToSamples( SPEC_OVD_MIN_ZERO_PULSE - MARGIN_INSIDE_OVD_ZERO_PULSE ); //6us spec
								maximum_pulse_width_one = UsToSamples( SPEC_MAX_OVD_ONE_PULSE + MARGIN_OUTSIDE_OVD_ONE_PULSE ); //2us spec
							}
							if ( mLowPulseLength > minimum_pulse_width_zero )
							{
								//store a zero into the data.
							}
							else if( mLowPulseLength <= maximum_pulse_width_one )
							{
								//store a one into the data.
								mRomDetected |= U64(0x1) << ((mRomBitsRecieved - 2) / 3);
							}
							else
							{
								mCurrentState = UnknownState;
								//Something Broke.
							}
							*/
							U64 sample_location_offset = UsToSamples( SPEC_SAMPLE_POINT );
							if( mOverdrive == true )
								sample_location_offset = UsToSamples( SPEC_OVD_SAMPLE_POINT );
							mOneWire->Advance( sample_location_offset );
							
							if( mOneWire->GetBitState() == BIT_HIGH )
							{
								//short pulse, this is a 1
								mRomDetected |= U64(0x1) << ((mRomBitsRecieved - 2) / 3);
							}
							else
							{
								//long pulse, this is a 0
							}
						}
						mRomBitsRecieved += 1;


						if( mRomBitsRecieved == 24 )
							RecordFrame( mByteStartSample, mRisingEdgeSample, FamilyCode, mRomDetected );

						if( mRomBitsRecieved == 168 )
							RecordFrame( mByteStartSample, mRisingEdgeSample, Rom, ((mRomDetected >> 8) & 0xFFFFFFFFFFFFull) );

						if( mRomBitsRecieved == 192 )
						{
							//Bubble the ROM CRC.
							//TODO:  RecordBubble( one_wire_bubbles, mByteStartSample, mRisingEdgeSample, CRC, (mRomDetected >> 56) );
							RecordFrame( mByteStartSample, mRisingEdgeSample, CRC, (mRomDetected >> 56) );
							//Move to the next state
							mCurrentState = RomFinishedState;
						}
					}
					break;
				case MatchRom:
					{
						//expecting 64 bits of rom address, nothing more.
						//mRomDetected
						if( mRomBitsRecieved == 8 )
						{
							//Bubble the ROM Family Code.
							//TODO:  RecordBubble( one_wire_bubbles, mByteStartSample, mPreviousRisingEdgeSample, FamilyCode, mRomDetected );
							//RecordFrame( mByteStartSample, mPreviousRisingEdgeSample, FamilyCode, mRomDetected );
							mByteStartSample = mFallingEdgeSample;
						}
						if( mRomBitsRecieved == 56 )
						{
							//Bubble the ROM Data bits.
							//TODO:  RecordBubble( one_wire_bubbles, mByteStartSample, mPreviousRisingEdgeSample, Rom, ((mRomDetected >> 8) & 0xFFFFFFFFFFFFull) );
							//RecordFrame( mByteStartSample, mPreviousRisingEdgeSample, Rom, ((mRomDetected >> 8) & 0xFFFFFFFFFFFFull) );
							mByteStartSample = mFallingEdgeSample;
						}
						if( mRomBitsRecieved == 0 )
						{
							//Tag start of Family Code.
							mByteStartSample = mFallingEdgeSample;
						}
						/*
						U64 minimum_pulse_width_zero = UsToSamples( SPEC_MIN_ZERO_PULSE - MARGIN_INSIDE_ZERO_PULSE ); //60us spec
						U64 maximum_pulse_width_one = UsToSamples( SPEC_MAX_ONE_PULSE + MARGIN_OUTSIDE_ONE_PULSE ); //15us spec
						if( mOverdrive == true )
						{
							minimum_pulse_width_zero = UsToSamples( SPEC_OVD_MIN_ZERO_PULSE - MARGIN_INSIDE_OVD_ZERO_PULSE ); //6us spec
							maximum_pulse_width_one = UsToSamples( SPEC_MAX_OVD_ONE_PULSE + MARGIN_OUTSIDE_OVD_ONE_PULSE ); //2us spec
						}
						if ( mLowPulseLength > minimum_pulse_width_zero )
						{
							//store a zero into the data.
						}
						else if( mLowPulseLength <= maximum_pulse_width_one )
						{
							//store a one into the data.
							mRomDetected |= U64(0x1) << mRomBitsRecieved;
						}
						else
						{
							mCurrentState = UnknownState;
							//Something Broke.
						}
						*/
						U64 sample_location_offset = UsToSamples( SPEC_SAMPLE_POINT );
						if( mOverdrive == true )
							sample_location_offset = UsToSamples( SPEC_OVD_SAMPLE_POINT );
						mOneWire->Advance( sample_location_offset );
						
						if( mOneWire->GetBitState() == BIT_HIGH )
						{
							//short pulse, this is a 1
							mRomDetected |= U64(0x1) << mRomBitsRecieved;
						}
						else
						{
							//long pulse, this is a 0
						}
						mRomBitsRecieved += 1;

						if( mRomBitsRecieved == 8 )
							RecordFrame( mByteStartSample, mRisingEdgeSample, FamilyCode, mRomDetected );

						if( mRomBitsRecieved == 56 )
							RecordFrame( mByteStartSample, mRisingEdgeSample, Rom, ((mRomDetected >> 8) & 0xFFFFFFFFFFFFull) );

						if( mRomBitsRecieved == 64 )
						{
							//Bubble the ROM CRC.
							//TODO:  RecordBubble( one_wire_bubbles, mByteStartSample, mRisingEdgeSample, CRC, (mRomDetected >> 56) );
							RecordFrame( mByteStartSample, mRisingEdgeSample, CRC, (mRomDetected >> 56) );
							//Move to the next state
							mCurrentState = RomFinishedState;

						}
					}
					break;
				case OverdriveSkipRom:
					{
						mCurrentState = RomFinishedState;
						mOverdrive = true;
						//we cannot through away our current transision!
						mBlockPulseAdvance = true;
					}
					break;
				case OverdriveMatchRom:
					{
						mOverdrive = true;
						//expecting 64 bits of rom address.
						mCurrentRomCommand = MatchRom;
						//we cannot through away our current transision!
						mBlockPulseAdvance = true;
						
					}
					break;
				}

			}
			break;
		case RomFinishedState:
			{			
				if( mDataBitsRecieved == 0 )
				{
					mDataDetected = 0;
					//Tag start of Family Code.
					mByteStartSample = mFallingEdgeSample;
				}
				/*
				U64 minimum_pulse_width_zero = UsToSamples( SPEC_MIN_ZERO_PULSE - MARGIN_INSIDE_ZERO_PULSE ); //60us spec
				U64 maximum_pulse_width_one = UsToSamples( SPEC_MAX_ONE_PULSE + MARGIN_OUTSIDE_ONE_PULSE ); //15us spec
				if( mOverdrive == true )
				{
					minimum_pulse_width_zero = UsToSamples( SPEC_OVD_MIN_ZERO_PULSE - MARGIN_INSIDE_OVD_ZERO_PULSE ); //6us spec
					maximum_pulse_width_one = UsToSamples( SPEC_MAX_OVD_ONE_PULSE + MARGIN_OUTSIDE_OVD_ONE_PULSE ); //2us spec
				}
				if ( mLowPulseLength > minimum_pulse_width_zero )
				{
					//store a zero into the data.
				}
				else if( mLowPulseLength <= maximum_pulse_width_one )
				{
					//store a one into the data.
					mDataDetected |= U64(0x1) << mDataBitsRecieved;
				}
				else
				{
					mCurrentState = UnknownState;
					//Something Broke.
				}
				*/
				U64 sample_location_offset = UsToSamples( SPEC_SAMPLE_POINT );
				if( mOverdrive == true )
					sample_location_offset = UsToSamples( SPEC_OVD_SAMPLE_POINT );
				mOneWire->Advance( sample_location_offset );
				
				if( mOneWire->GetBitState() == BIT_HIGH )
				{
					//short pulse, this is a 1
					mDataDetected |= U64(0x1) << mDataBitsRecieved;
				}
				else
				{
					//long pulse, this is a 0
				}
				mDataBitsRecieved += 1;

				if( mDataBitsRecieved == 8 )
				{
					//Bubble the ROM Family Code.
					//TODO:  RecordBubble( one_wire_bubbles, mByteStartSample, mRisingEdgeSample, Byte, mDataDetected );
					RecordFrame( mByteStartSample, mRisingEdgeSample, Byte, mDataDetected );
					mDataBitsRecieved = 0;
				}

			}
			break;
		}


/*
		if( low_pulse_time < 480 )
		{
			//not a reset pulse
			continue;
		}
		//RecordSlowResetPresence( one_wire_bubbles, falling_edge_sample, rising_edge_sample,  );
		
		ResultBubble one_wire_bubble_reset;
		one_wire_bubble_reset.mStartingSampleInclusive = falling_edge_sample;
		one_wire_bubble_reset.mEndingSampleInclusive = rising_edge_sample;
		one_wire_bubble_reset.mData1 = U64( RestartPulse );
		one_wire_bubbles.push_back( one_wire_bubble_reset );

		one_wire.MoveRightUntilBitChanges(true);

		falling_edge_sample = one_wire.GetSampleNumber();
		high_pulse_length = falling_edge_sample - rising_edge_sample;
		high_pulse_time = high_pulse_length * 1000000 / mSampleRateHz; //micro seconds
		
		if( high_pulse_time > 60 )
		{
			one_wire.MoveLeftUntilBitChanges(); //move back to the rising transision.
			continue;
		}
		
		one_wire.MoveRightUntilBitChanges(true);
		rising_edge_sample = one_wire.GetSampleNumber();
		low_pulse_length = rising_edge_sample - falling_edge_sample;
		low_pulse_time = low_pulse_length * 1000000 / mSampleRateHz; //micro seconds

		if( (( low_pulse_time > 60 ) && (low_pulse_time < 240)) != true )
		{
			//not a presence pulse
			continue;
		}
		ResultBubble presence_bubble;
		presence_bubble.mStartingSampleInclusive = falling_edge_sample;
		presence_bubble.mEndingSampleInclusive = rising_edge_sample;
		presence_bubble.mData1 = U64( PresencePulse );
		one_wire_bubbles.push_back( presence_bubble );

		one_wire_bubbles.SetSafeEnd();
		//woot! lets get this game started!
		//U8 data;
		*/
		
		//one_wire_bubbles.SetSafeEnd();
		ReportProgress( mOneWire->GetSampleNumber() );
		CheckIfThreadShouldExit();
		//mProgressManager->ReportAlgorithmProgress( this, one_wire.GetSampleNumber() );

	}
}

void OneWireAnalyzer::RecordFrame( U64 starting_sample, U64 ending_sample, OneWireFrameType type, U64 data, bool warning )
{
	Frame frame;
	U8 flags = 0;
	if( warning == true )
		flags |= DISPLAY_AS_WARNING_FLAG;
	frame.mFlags = flags;
	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive = ending_sample;
	frame.mType = (U8)type;
	frame.mData1 = data;

	mResults->AddFrame( frame );

	mResults->CommitResults();
}

/*
void OneWireAnalyzer::RecordBubble( ChunkedArray<ResultBubble>& one_wire_bubbles, U64 sample_start, U64 sample_end, OneWireResultBubbleType type, U64 data2, U64 data3, U64 )
{

	ResultBubble one_wire_bubble;
	one_wire_bubble.mStartingSampleInclusive = sample_start;
	one_wire_bubble.mEndingSampleInclusive = sample_end;
	one_wire_bubble.mData1 = U64( type );
	
	//RestartPulse, PresencePulse, RomCommand, CRC, FamilyCode, Rom, Byte, Bit
	switch( type )
	{
	case RestartPulse:
		{
			one_wire_bubble.mData2 = data2;
		
		}
		break;
	case PresencePulse:
		{

		}
		break;
	case RomCommand:
		{

			//data2 = command enum
			one_wire_bubble.mData2 = data2;
			//data3 = command hex.
			one_wire_bubble.mData3 = data3;
		}
		break;
	case CRC:
		{
			one_wire_bubble.mData2 = data2;
		}
		break;
	case FamilyCode:
		{
			one_wire_bubble.mData2 = data2;
		}
		break;
	case Rom:
		{
			one_wire_bubble.mData2 = data2;
		}
		break;
	case Byte:
		{
			//data2 = data enum. (data enum not created yet)
			//data3 = byte
			one_wire_bubble.mData2 = data2; // temp, for now unitll we get enums.

		}
		break;
	case Bit:
		{
			LogicAssert( "Trying to record a bit bubble" );
			//data2 = binary value.
		}
		break;
	}

	one_wire_bubbles.push_back( one_wire_bubble );
	one_wire_bubbles.SetSafeEnd();

	
}
*/
/*
U32 OneWireAnalyzer::GetBubbleText( Channel& channel, U32 index, DisplayBase display_base, const char ** bubble_text, U32 array_size  )
{
	if( mResultBubbles.find( channel ) == mResultBubbles.end() )
		LogicAssert( "GetBubblePosition - channel isn't in map" );

	if( array_size == 0 )
		LogicAssert( "Trying to request zero bubble text items" );

	ResultBubble& bubble = mResultBubbles[channel][index];

	mTextVariants.clear();

	OneWireResultBubbleType type = OneWireResultBubbleType( bubble.mData1 );
	//RestartPulse, PresencePulse, RomCommand, CRC, FamilyCode, Rom, Byte, Bit 
	switch( type )
	{
	case RestartPulse :
		{
			if( bubble.mData2 == 0 )
			{
				mTextVariants.push_back( "R" );
				mTextVariants.push_back( "RESET" );
				mTextVariants.push_back( "RESET condition" );
			}
			else
			{
				mTextVariants.push_back( "R!" );
				mTextVariants.push_back( "RESET - WARNING" );
				mTextVariants.push_back( "RESET - warning, too short." );
				mTextVariants.push_back( "RESET - warning, pulse shorter than 480us" );	
			}
		}
		break;
	case PresencePulse:
		{
			mTextVariants.push_back( "P" );
			mTextVariants.push_back( "PRESENCE" );
			mTextVariants.push_back( "PRESENCE condition" );
		}
		break;
	case RomCommand:
		{
			//ReadRom, SkipRom, SearchRom, MatchRom, OverdriveSkipRom, OverdriveMatchRom
			//data2 = command enum
			//data3 = command hex.
			std::string number_str = AnalyzerHelpers::GetNumberString( bubble.mData3, display_base, 8 );
			switch( OneWireRomCommand( bubble.mData2 ) )
			{
			case ReadRom:
				{
					mTextVariants.push_back( "READ" );
					mTextVariants.push_back( "READ ROM COMMAND" );
					mTextVariants.push_back( "READ ROM command: [" + number_str + "]" );
				}
				break;
			case SkipRom:
				{
					mTextVariants.push_back( "SKIP" );
					mTextVariants.push_back( "SKIP ROM COMMAND" );
					mTextVariants.push_back( "SKIP ROM command: [" + number_str + "]" );
				}
				break;
			case SearchRom:
				{
					mTextVariants.push_back( "SEARCH" );
					mTextVariants.push_back( "SEARCH ROM COMMAND" );
					mTextVariants.push_back( "SEARCH ROM command: [" + number_str + "]" );
				}
				break;
			case MatchRom:
				{
					mTextVariants.push_back( "MATCH" );
					mTextVariants.push_back( "MATCH ROM COMMAND" );
					mTextVariants.push_back( "MATCH ROM command: [" + number_str + "]" );
				}
				break;
			case OverdriveSkipRom:
				{
					mTextVariants.push_back( "OD SKIP" );
					mTextVariants.push_back( "OVERDRIVE SKIP ROM" );
					mTextVariants.push_back( "OVERDRIVE SKIP ROM command: [" + number_str + "]" );
				}
				break;
			case OverdriveMatchRom:
				{
					mTextVariants.push_back( "OD MATCH" );
					mTextVariants.push_back( "OVERDRIVE MATCH COMMAND" );
					mTextVariants.push_back( "OVERDRIVE MATCH ROM command: [" + number_str + "]" );
				}
				break;
			}
		}
		break;
	case CRC:
		{
			std::string crc_string = AnalyzerHelpers::GetNumberString( bubble.mData2, display_base, 8 );
			mTextVariants.push_back( "CRC" );
			mTextVariants.push_back( "CRC: [" + crc_string +"]" );
			mTextVariants.push_back( "CRC section from ROM: [" + crc_string + "]" );
		}
		break;
	case FamilyCode:
		{
			std::string family_string = AnalyzerHelpers::GetNumberString( bubble.mData2, display_base, 8 );
			mTextVariants.push_back( "FAMILY" );
			mTextVariants.push_back( "FAMILY: [" + family_string + "]" );
			mTextVariants.push_back( "FAMILY CODE section from ROM: [" + family_string + "]" );
		}
		break;
	case Rom:
		{
			std::string rom_string = AnalyzerHelpers::GetNumberString( bubble.mData2, display_base, 48 );
			mTextVariants.push_back( "ROM" );
			mTextVariants.push_back( "ROM: [" + rom_string +"]" );
			mTextVariants.push_back( "ROM CODE section from ROM: [" + rom_string + "]" );
		}
		break;
	case Byte:
		{
			std::string data_string = AnalyzerHelpers::GetNumberString( bubble.mData2, display_base, 8 );
			mTextVariants.push_back( "D" );
			mTextVariants.push_back( "DATA" );
			mTextVariants.push_back( "DATA: [" + data_string + "]" );
		}
		break;
	case Bit:
		{
			mTextVariants.push_back( "Z" );
			mTextVariants.push_back( "BIT" );
			mTextVariants.push_back( "BIT - ERROR." );
		}
		break;
	}

	////////////////////////////////////////////
	
	U32 num_bubbles = mTextVariants.size();
	if( num_bubbles > array_size )
		num_bubbles = array_size;

	if( num_bubbles == 0 )
		LogicAssert( "produced zero strings!" );

	for( U32 i=0; i<num_bubbles; i++ )
	{
		bubble_text[i] = mTextVariants[i].c_str();
	}

	return num_bubbles;
}
*/

/*
const char* OneWireAnalyzer::GetSimpleExportText( DisplayBase display_base )
{
	if( !mProgressManager )
		LogicAssert( "called GetSimpleExportText at invalid time" );

	//create CSV results.
	mSimpleExportText.clear();
	std::stringstream ss(std::stringstream::in | std::stringstream::out);
	//push back the header.

	ss << "Time[s], Detail, [data]" << "\n";

	//ss.setf( 0, std::ios::floatfield );
	ss.precision( 15 );

	ChunkedArray<ResultBubble>& one_wire_bubbles = mResultBubbles[ mOneWireSettings->mOneWireChannel ];

	ChunkedArray<ResultBubble>::iterator it = one_wire_bubbles.begin();

	if( mProgressManager->IsTriggered() == false )
		LogicAssert( "Exporting Analyzer data when data is not triggered" );

	U64 trigger_sample = mProgressManager->GetTriggerSample();

	for( ; it != one_wire_bubbles.end(); ++it )
	{
		//for each result bubble, push back one line of text.
		// time, detail, data.
		OneWireResultBubbleType type = OneWireResultBubbleType( it->mData1 );
		//RestartPulse, PresencePulse, RomCommand, CRC, FamilyCode, Rom, Byte, Bit 

		AnalyzerHelpers::GetTimeString( it->mStartingSampleInclusive, trigger_sample, mSampleRateHz, ss );
		ss << ",";


		switch( type )
		{
		case RestartPulse:
			{
				if( it->mData2 == 0 ) //correct reset pulse.
				{
					ss << "Reset Pulse";
				}
				else //out of spec reset pulse.
				{
					ss << "Reset Pulse ( out of spec )";
				}
			}
			break;
		case PresencePulse:
			{
				ss << "Presence Pulse";
			}
			break;
		case RomCommand:
			{
				//ReadRom, SkipRom, SearchRom, MatchRom, OverdriveSkipRom, OverdriveMatchRom
				//data2 = command enum
				//data3 = command hex.
				std::string number_str = AnalyzerHelpers::GetNumberString( it->mData3, display_base, 8 );
				switch( OneWireRomCommand( it->mData2 ) )
				{
				case ReadRom:
					{
						ss << "Read Rom Command" << ", " << number_str;
					}
					break;
				case SkipRom:
					{
						ss << "Skip Rom Command" << ", " << number_str;
					}
					break;
				case SearchRom:
					{
						ss << "Search Rom Command" << ", " << number_str;
					}
					break;
				case MatchRom:
					{
						ss << "Match Rom Command" << ", " << number_str;
					}
					break;
				case OverdriveSkipRom:
					{
						ss << "Overdrive Skip Rom Command" << ", " << number_str;
					}
					break;
				case OverdriveMatchRom:
					{
						ss << "Overdrive Match Rom Command" << ", " << number_str;
					}
					break;
				}
			}
			break;
		case CRC:
			{	
				std::string crc_string = AnalyzerHelpers::GetNumberString( it->mData2, display_base, 8 );
				ss << "ROM CRC" << ", " << crc_string;
			}
			break;
		case FamilyCode:
			{
				std::string family_string = AnalyzerHelpers::GetNumberString( it->mData2, display_base, 8 );
				ss << "ROM Family Code" << ", " << family_string;
			}
			break;
		case Rom:
			{
				std::string rom_string = AnalyzerHelpers::GetNumberString( it->mData2, display_base, 48 );
				ss << "ROM Code" << ", " << rom_string;

			}
			break;
		case Byte:
			{
				std::string data_string = AnalyzerHelpers::GetNumberString( it->mData2, display_base, 8 );
				ss << "Data" << ", " << data_string;
			}
			break;
		case Bit:
			{
			}
			break;
		}
		ss << "\n";
	}
	mSimpleExportText = ss.str();
	return mSimpleExportText.c_str();
}
*/
U32 OneWireAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{

	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );

	/*
	if( mSimulationChannels.empty() == true )
	{
		//init Simulation channel descriptors

		SimulationChannelDescriptor one_wire_channel;
		one_wire_channel.mChannel = mOneWireSettings->mOneWireChannel;
		one_wire_channel.mSampleRate = mSimulationSampleRateHz;
		one_wire_channel.mInitialBitState = BIT_HIGH;
		//one_wire_channel.mChannelData.reset( new ChunkedArray< U64 >() );
		mSimulationChannels.push_back( one_wire_channel );

		mOneWireTransitions = mSimulationChannels[0].mChannelData;

		mSimulationSampleIndex = 1000;

		mSimOverdrive = false;
	}
	if( sample_rate < mSimulationSampleRateHz )
		LogicAssert( "Invalid sample rates. %u, %u", sample_rate, mSimulationSampleRateHz );

	U64 multiplier = sample_rate / mSimulationSampleRateHz;
	if( sample_rate % mSimulationSampleRateHz != 0 )
		LogicAssert( "Invalid sample rates. %u, %u", sample_rate, mSimulationSampleRateHz );

	U64 adjusted_minimum_sample_index = newest_sample_requested / multiplier;

	U32 delay;
	while( mSimulationSampleIndex < adjusted_minimum_sample_index )
	{
		//CreateSerialByte( 0x55, mSimulationChannels[0].mChannelData );
		if(mSimOverdrive == true)
			delay = 30;
		else
			delay = 250;

		mSimulationSampleIndex += UsToSamples(delay * 3, true);

		SimResetPacket();
		SimReadRom(0x8877665544332211ull);

		mSimulationSampleIndex += UsToSamples(delay, true);
		SimWriteByte(0x37);
		mSimulationSampleIndex += UsToSamples(delay, true);
		SimWriteByte(0xF0);

		std::vector<U64> device_ROMs;
		device_ROMs.push_back(0x8877665544332211ull);
		device_ROMs.push_back(0x1122334455667788ull);
		
		SimResetPacket();
		U64 found_device;
		found_device = SimSearchRom( device_ROMs );

		mSimulationSampleIndex += UsToSamples(delay, true);
		SimWriteByte(0x0F);
		mSimulationSampleIndex += UsToSamples(delay, true);
		SimWriteByte(0xF0);
		mSimulationSampleIndex += UsToSamples(delay, true);
		SimWriteByte(0x55);
		mSimulationSampleIndex += UsToSamples(delay, true);
		SimWriteByte(0x18);
		
		SimResetPacket();
		SimMatchRom(0xF0E1D2C3B4A59687ull);

		mSimulationSampleIndex += UsToSamples(delay, true);
		SimWriteByte(0x55);
		mSimulationSampleIndex += UsToSamples(delay, true);
		SimWriteByte(0x18);
				
		if( mSimOverdrive == true )
			mSimOverdrive = false;
		else
		{
			SimResetPacket();
			SimOverdriveSkipRom();
		}
	
	}

	*simulation_channel = &mSimulationChannels[0];
	return mSimulationChannels.size();

	*/
}
bool OneWireAnalyzer::NeedsRerun()
{
	return false;
}

U64 OneWireAnalyzer::UsToSamples( U64 us )
{

	return ( mSampleRateHz * us ) / 1000000;


}

U64 OneWireAnalyzer::SamplesToUs( U64 samples )
{
	return( samples * 1000000 ) / mSampleRateHz;
}

U32 OneWireAnalyzer::GetMinimumSampleRateHz()
{
	return 2000000;
}

const char gAnalyzerName[] = "1-Wire";  //your analyzer must have a unique name

const char* OneWireAnalyzer::GetAnalyzerName() const
{
	return gAnalyzerName;
}

const char* GetAnalyzerName()
{
	return gAnalyzerName;
}

Analyzer* CreateAnalyzer()
{
	return new OneWireAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
