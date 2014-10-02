#pragma warning(push, 0)
#include <sstream>
#include <ios>
#pragma warning(pop)

#include "OneWireAnalyzer.h"
#include "OneWireAnalyzerSettings.h"  
#include <AnalyzerChannelData.h>


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
	//mResults->AddChannelBubblesWillAppearOn( mSettings->mOneWireChannel );

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
			min_low_pulse_samples /= 2; //customer had data where the min pulse was about 0.8us.
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
				mOneWire->Advance( U32( sample_location_offset ) );
				
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
					else if( (mDataDetected == 0xF0) || (mDataDetected == 0x0F) )
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

						U64 sample_location_offset = UsToSamples( SPEC_SAMPLE_POINT );
						if( mOverdrive == true )
							sample_location_offset = UsToSamples( SPEC_OVD_SAMPLE_POINT );
						mOneWire->Advance( U32( sample_location_offset ) );
						
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
							mOneWire->Advance( U32( sample_location_offset ) );
							
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

						U64 sample_location_offset = UsToSamples( SPEC_SAMPLE_POINT );
						if( mOverdrive == true )
							sample_location_offset = UsToSamples( SPEC_OVD_SAMPLE_POINT );
						mOneWire->Advance( U32( sample_location_offset ) );
						
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

				U64 sample_location_offset = UsToSamples( SPEC_SAMPLE_POINT );
				if( mOverdrive == true )
					sample_location_offset = UsToSamples( SPEC_OVD_SAMPLE_POINT );
				mOneWire->Advance( U32( sample_location_offset ) );
				
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


		ReportProgress( mOneWire->GetSampleNumber() );
		CheckIfThreadShouldExit();


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



U32 OneWireAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{

	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );


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
