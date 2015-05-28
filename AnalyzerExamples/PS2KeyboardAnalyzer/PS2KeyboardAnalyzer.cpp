#include "PS2KeyboardAnalyzer.h"
#include "PS2KeyboardAnalyzerSettings.h"
#include "PS2KeyboardAnalyzerScanCodes.h"
#include <AnalyzerHelpers.h>
#include <AnalyzerChannelData.h>

PS2KeyboardAnalyzer::PS2KeyboardAnalyzer()
:	Analyzer2(),
	mSettings( new PS2KeyboardAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
	
}

PS2KeyboardAnalyzer::~PS2KeyboardAnalyzer()
{
	KillThread();
}

void PS2KeyboardAnalyzer::WorkerThread()
{
	mData = GetAnalyzerChannelData( mSettings->mDataChannel );
	mClock = GetAnalyzerChannelData( mSettings->mClockChannel );

	//begin from here
	for( ; ; )
	{	
		//init all variables, these will be updated by the GetNextData function

		U64 DataPayload =0;
		U64 frame_starting_sample;
		U64 frame_ending_sample;
		
		bool DeviceTx = false;
		bool ParityError = false;
		bool ACKed = false;
		
		//get a data transmission
		GetNextData(frame_starting_sample, frame_ending_sample, DeviceTx, DataPayload, ParityError, ACKed);
		
		Frame frame;	
		frame.mStartingSampleInclusive = frame_starting_sample;

		U8 flags =0x00;
		
		//begin to analyze the frame based on direction of transmission
		if(DeviceTx)
		{
			if(mSettings->mDeviceType==0)
			{
				//transmission is from Device->Host, device is keyboard
				flags = flags | TX_DEVICE_TO_HOST;
				
				bool EndOfFrame = false;

				while(!EndOfFrame)
				{
					if(DataPayload==0xE0)
					{
						//extended key code
						flags = flags | EXTENDED_KEY;
						GetNextData(frame_starting_sample, frame_ending_sample, DeviceTx, DataPayload, ParityError, ACKed);
						EndOfFrame=false;
					}
					else if (DataPayload==0xEE)
					{
						flags = flags | DATA_FRAME;
						frame.mData2 = ECHO_FRAME;
						EndOfFrame = true;
					}
					else if (DataPayload==0xAA)
					{
						flags = flags | DATA_FRAME;
						frame.mData2 = BAT_FRAME;
						EndOfFrame = true;
					}
					else if (DataPayload==0xFA)
					{
						flags = flags | DATA_FRAME;
						frame.mData2 = ACK_FRAME;
						EndOfFrame = true;
					}
					else if(DataPayload==0xF0)
					{
						//break code
						flags = flags | BREAK_CODE;
						GetNextData(frame_starting_sample, frame_ending_sample, DeviceTx, DataPayload, ParityError, ACKed);
						EndOfFrame=false;
					}
					else if(DataPayload==0xE1)
					{
						//Pause/break key
						bool IsErrorInTx = false;
						bool finished = false;
						U64 compare_data[7] = {0x14,0x77,0xE1,0xF0,0x14,0xF0,0x77};
						int cnt=0;

						while(!IsErrorInTx && !finished)
						{
							GetNextData(frame_starting_sample, frame_ending_sample, DeviceTx, DataPayload, ParityError, ACKed);
							
							if(compare_data[cnt]!=DataPayload)
								IsErrorInTx = true;
							else
								cnt++;

							if(cnt>6)
								finished=true;
						}

						if(finished)
							flags = flags | PAUSE_BREAK;
						else
							flags = flags | ERROR_FRAME;
						
						EndOfFrame=true;
					}
					else if(DataPayload==0x12 && (flags&EXTENDED_KEY))
					{
						//Print Screen Make
						bool IsErrorInTx = false;
						bool finished = false;
						U64 compare_data[2] = {0xE0,0x7C};
						int cnt=0;

						while(!IsErrorInTx && !finished)
						{
							GetNextData(frame_starting_sample, frame_ending_sample, DeviceTx, DataPayload, ParityError, ACKed);
							
							if(compare_data[cnt]!=DataPayload)
								IsErrorInTx = true;
							else
								cnt++;

							if(cnt>1)
								finished=true;
						}

						if(finished)
						{
							flags = flags | PRINT_SCREEN | MAKE_CODE;
						}
						else
							flags = flags | ERROR_FRAME;
						
						EndOfFrame=true;
					}
					else if(DataPayload==0x7C && flags&BREAK_CODE && flags&EXTENDED_KEY)
					{
						//Print Screen Break
						bool IsErrorInTx = false;
						bool finished = false;
						U64 compare_data[3] = {0xE0,0xF0,0x12};
						int cnt=0;

						while(!IsErrorInTx && !finished)
						{
							GetNextData(frame_starting_sample, frame_ending_sample, DeviceTx, DataPayload, ParityError, ACKed);
							
							if(compare_data[cnt]!=DataPayload)
								IsErrorInTx = true;
							else
								cnt++;

							if(cnt>2)
								finished=true;
						}

						if(finished)
						{
							flags = flags | PRINT_SCREEN | BREAK_CODE;
						}
						else
							flags = flags | ERROR_FRAME;
						
						EndOfFrame=true;
					}
					else
					{
						//value
						EndOfFrame=true;
					}
				}

				frame.mData1 = DataPayload;
			}
			else
			{
				//device is a mouse, transmission from device to house
				flags = flags | TX_DEVICE_TO_HOST;
		
					if(DataPayload==0xFA)
					{
						flags = flags | DATA_FRAME;
						frame.mData2 = ACK_FRAME;
					}
					else if (DataPayload==0xAA)
					{
						flags = flags | DATA_FRAME;
						frame.mData2 = BAT_FRAME;
					}
					else if(DataPayload&0x08)
					{
						flags = flags | MOVEMENT_FRAME;

						U64 movement_packet[4] = {0x00, 0x00, 0x00, 0x00};
						movement_packet[0] = DataPayload;

						GetNextData(frame_starting_sample, frame_ending_sample, DeviceTx, movement_packet[1], ParityError, ACKed);

						GetNextData(frame_starting_sample, frame_ending_sample, DeviceTx, movement_packet[2], ParityError, ACKed);

						if(mSettings->mDeviceType==2)
							GetNextData(frame_starting_sample, frame_ending_sample, DeviceTx, movement_packet[3], ParityError, ACKed);					
						
						DataPayload = movement_packet[3]<<24 | movement_packet[2]<<16 | movement_packet[1]<<8 | movement_packet[0];
					}
					else
					{

					}
				frame.mData1 = DataPayload;
			}
		}
		else
		{
			//transmission is Host->Device
			flags = flags | TX_HOST_TO_DEVICE;
			frame.mData1 = DataPayload;

		}
		frame.mFlags = flags;
		frame.mEndingSampleInclusive = frame_ending_sample;
		
		mResults->AddFrame(frame);
		mResults->CommitResults();

		ReportProgress( frame.mEndingSampleInclusive );
		CheckIfThreadShouldExit();

	}

}

bool PS2KeyboardAnalyzer::NeedsRerun()
{
	return false;
}

U32 PS2KeyboardAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 PS2KeyboardAnalyzer::GetMinimumSampleRateHz()
{
    return 25000;
}

void PS2KeyboardAnalyzer::SetupResults()
{
    mResults.reset( new PS2KeyboardAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mDataChannel );
}

const char* PS2KeyboardAnalyzer::GetAnalyzerName() const
{
	return "PS/2 Keyboard/Mouse";
}

const char* GetAnalyzerName()
{
	return "PS/2 Keyboard/Mouse";
}

Analyzer* CreateAnalyzer()
{
	return new PS2KeyboardAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}

void PS2KeyboardAnalyzer::GetNextData(U64 &starting_frame, U64 &ending_frame, bool &DeviceToHost, U64 &Payload, bool &ParityError, bool &ACKed)
{
	//Function built by looking at waveforms located here: http://retired.beyondlogic.org/keyboard/keybrd.htm
	//Note that there is conflicting information on the internet concerning the most popular websites for this protocol
	//some indicate that data is always sampled on falling clock edges, others indicate host to device communication
	//should be read on rising edges. Experimentation with Sony PS/2 PC and keyboard indicate host-to-device must be
	//read on rising clock edges, confirming some of the sources I found on the internet
		
	//if theres a transition in data before the clock goes high, request to send
	bool gotdata = false;
	while(!gotdata)
	{
		if(mClock->GetBitState()==BIT_LOW && mData->WouldAdvancingToAbsPositionCauseTransition(mClock->GetSampleOfNextEdge()) && mData->GetBitState()==BIT_HIGH)
			DeviceToHost=false;
		else
		{
			//advance until there is some activity on the channels
			while(mClock->GetBitState()==BIT_HIGH && mData->GetBitState()==BIT_HIGH)
			{
				U64 NextEdgeData = mData->GetSampleOfNextEdge();
				U64 NextEdgeClock = mClock->GetSampleOfNextEdge();

				if(NextEdgeData<NextEdgeClock)
				{
					mClock->AdvanceToAbsPosition(NextEdgeData);
					mData->AdvanceToAbsPosition(NextEdgeData);
				}
				else
				{
					mClock->AdvanceToAbsPosition(NextEdgeClock);
					mData->AdvanceToAbsPosition(NextEdgeClock);
				}
			}
			
			//if the first activity is a Request-To-Send, then the transmission will be from Host to Device
			if(mClock->GetBitState()==BIT_LOW && mData->WouldAdvancingToAbsPositionCauseTransition(mClock->GetSampleOfNextEdge()) && mData->GetBitState()==BIT_HIGH)
				DeviceToHost=false;
			else
				DeviceToHost=true;
		}

		//now we know direction, lets start getting the data
		mClock->AdvanceToNextEdge();
		mData->AdvanceToAbsPosition(mClock->GetSampleNumber());

		//Now we are at the start bit of a transmission
		U64 current_location = mClock->GetSampleNumber();
		starting_frame = current_location;

		//the start bit is always 0, make sure this is zero, otherwise mark it as an error
		if(mData->GetBitState()==BIT_LOW)
		{

			//valid start bit, mark edge of clock and mark start bit with green circle
			mResults->AddMarker( current_location, AnalyzerResults::Start, mSettings->mDataChannel );
			
			if(DeviceToHost)
				mResults->AddMarker( current_location, AnalyzerResults::DownArrow, mSettings->mClockChannel );
			else
				mResults->AddMarker( current_location, AnalyzerResults::UpArrow, mSettings->mClockChannel );

			//prepare to collect the data
			U64 data = 0;
			DataBuilder data_builder;
			data_builder.Reset( &data, AnalyzerEnums::LsbFirst, 8 );
				
			//loop through the 8 bits of payload
			for( U32 i=0; i<8; i++ )
			{
				//advance to the payload bits beginning at the next falling edge
				mClock->AdvanceToNextEdge();
				mClock->AdvanceToNextEdge();
				current_location = mClock->GetSampleNumber();

				mData->AdvanceToAbsPosition(current_location);
			
				//mark the clock edge

				if(DeviceToHost)
					mResults->AddMarker( current_location, AnalyzerResults::DownArrow, mSettings->mClockChannel );
				else
					mResults->AddMarker( current_location, AnalyzerResults::UpArrow, mSettings->mClockChannel );
				
				//mark the data channel
				mResults->AddMarker( current_location, AnalyzerResults::Dot, mSettings->mDataChannel );
				
				//read the value of the bit
				data_builder.AddBit(mData->GetBitState());
			}

			//set Payload = data for return variable
			Payload = data;

			//advance to the parity bit
			mClock->AdvanceToNextEdge();
			mClock->AdvanceToNextEdge();
			current_location = mClock->GetSampleNumber();

			//mark the clock edge
			if(DeviceToHost)
				mResults->AddMarker( current_location, AnalyzerResults::DownArrow, mSettings->mClockChannel );
			else
				mResults->AddMarker( current_location, AnalyzerResults::UpArrow, mSettings->mClockChannel );
			mData->AdvanceToAbsPosition(current_location);

			//read the parity bit from the stream and compare
			int received_parity;
			received_parity = mData->GetBitState();

			//now get expected value of parity bit
			if(AnalyzerHelpers::IsOdd( AnalyzerHelpers::GetOnesCount(data)))
			{
				if(received_parity==BIT_LOW)
				{
					//parity is correct
					mResults->AddMarker( current_location, AnalyzerResults::Square, mSettings->mDataChannel );
					ParityError = false;
				}
				else
				{
					//parity is incorrect
					mResults->AddMarker( current_location, AnalyzerResults::ErrorX, mSettings->mDataChannel );
					ParityError = true;
				}
			}
			else
			{		
				if(received_parity==BIT_HIGH)
				{
					//parity is correct
					mResults->AddMarker( current_location, AnalyzerResults::Square, mSettings->mDataChannel );
					ParityError = false;
				}
				else
				{
					//parity is incorrect
					mResults->AddMarker( current_location, AnalyzerResults::ErrorX, mSettings->mDataChannel );
					ParityError = true;
				}

			}

			//advance to the stop bit
			mClock->AdvanceToNextEdge();
			mClock->AdvanceToNextEdge();
			current_location = mClock->GetSampleNumber();
			mData->AdvanceToAbsPosition(current_location);
			
			//mark the clock edge
			if(DeviceToHost)
				mResults->AddMarker( current_location, AnalyzerResults::DownArrow, mSettings->mClockChannel );
			else
				mResults->AddMarker( current_location, AnalyzerResults::UpArrow, mSettings->mClockChannel );

			//make sure the stop bit is 1
			if(mData->GetBitState()==BIT_HIGH)
				mResults->AddMarker( current_location, AnalyzerResults::Stop, mSettings->mDataChannel );
			else
				mResults->AddMarker( current_location, AnalyzerResults::ErrorX, mSettings->mDataChannel );
					

			//if the transmit direction is host to device, check for the ACK
			if(!DeviceToHost) 
			{
				//check for ACK from Device, read on falling edge
				mClock->AdvanceToNextEdge();
				mClock->AdvanceToNextEdge();
				current_location = mClock->GetSampleNumber();
				mData->AdvanceToAbsPosition(current_location);
				
				//mark the clock edge
				mResults->AddMarker( current_location, AnalyzerResults::UpArrow, mSettings->mClockChannel );

				//Ack should be 0, otherwise error
				if(mData->GetBitState()==BIT_HIGH)
				{
					ACKed = true;
					mResults->AddMarker( current_location, AnalyzerResults::ErrorX, mSettings->mDataChannel );
				}
				else
				{
					ACKed = false;
					mResults->AddMarker( current_location, AnalyzerResults::Square, mSettings->mDataChannel );
				}
			
				//since ACK is read on falling cycle, we need to advance to the edge of this pulse
				mClock->AdvanceToNextEdge();
				current_location = mClock->GetSampleNumber();
				mData->AdvanceToAbsPosition(current_location);
			}
			
			//all done, the tranmission ended here
			ending_frame = mClock->GetSampleNumber();
			gotdata= true;
		}
		else
		{
			gotdata=false;
		}
	}
}
