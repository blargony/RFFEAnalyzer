#include "ModbusAnalyzer.h"
#include "ModbusAnalyzerSettings.h"
#include <AnalyzerChannelData.h>


ModbusAnalyzer::ModbusAnalyzer()
:	Analyzer2(),  
	mSettings( new ModbusAnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
	
	//used for calculating Modbus Checksum values
	init_crc16_tab();
}

ModbusAnalyzer::~ModbusAnalyzer()
{
	KillThread();
}

void ModbusAnalyzer::ComputeSampleOffsets()
{
	ClockGenerator clock_generator;
	clock_generator.Init( mSettings->mBitRate, mSampleRateHz );

	mSampleOffsets.clear();
	
	U32 num_bits = mSettings->mBitsPerTransfer;

	if( mSettings->mModbusMode != ModbusAnalyzerEnums::Normal )
	num_bits++;

	mSampleOffsets.push_back( clock_generator.AdvanceByHalfPeriod( 1.5 ) );  //point to the center of the 1st bit (past the start bit)
	num_bits--;  //we just added the first bit.

	for( U32 i=0; i<num_bits; i++ )
	{
		mSampleOffsets.push_back( clock_generator.AdvanceByHalfPeriod() );
	}

	if( mSettings->mParity != AnalyzerEnums::None )
		mParityBitOffset = clock_generator.AdvanceByHalfPeriod();

	//to check for framing errors, we also want to check
	//1/2 bit after the beginning of the stop bit
	mStartOfStopBitOffset = clock_generator.AdvanceByHalfPeriod( 1.0 );  //i.e. moving from the center of the last data bit (where we left off) to 1/2 period into the stop bit

	//and 1/2 bit before end of the stop bit period
	mEndOfStopBitOffset = clock_generator.AdvanceByHalfPeriod( mSettings->mStopBits - 1.0 );  //if stopbits == 1.0, this will be 0
}


void ModbusAnalyzer::SetupResults()
{
	//Unlike the worker thread, this function is called from the GUI thread
	//we need to reset the Results object here because it is exposed for direct access by the GUI, and it can't be deleted from the WorkerThread

	mResults.reset( new ModbusAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

void ModbusAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();
	ComputeSampleOffsets();
	U32 num_bits = mSettings->mBitsPerTransfer;

	if( mSettings->mModbusMode == ModbusAnalyzerEnums::MpModeMsbOneMeansAddress || mSettings->mModbusMode == ModbusAnalyzerEnums::MpModeMsbZeroMeansAddress)
		num_bits++;

	if( mSettings->mInverted == false )
	{
		mBitHigh = BIT_HIGH;
		mBitLow = BIT_LOW;
	}else
	{
		mBitHigh = BIT_LOW;
		mBitLow = BIT_HIGH;
	}

	U64 bit_mask = 0;
	U64 mask = 0x1ULL;
	for( U32 i=0; i<num_bits; i++ )
	{
		bit_mask |= mask;
		mask <<= 1;
	}
	
	mModbus = GetAnalyzerChannelData( mSettings->mInputChannel );
	mModbus->TrackMinimumPulseWidth();
	
	if( mModbus->GetBitState() == mBitLow )
		mModbus->AdvanceToNextEdge();

	//if Modbus isn't selected, use the other code untouched
	if(mSettings->mModbusMode != ModbusAnalyzerEnums::ModbusRTUMaster && mSettings->mModbusMode != ModbusAnalyzerEnums::ModbusRTUSlave && mSettings->mModbusMode != ModbusAnalyzerEnums::ModbusASCIIMaster && mSettings->mModbusMode != ModbusAnalyzerEnums::ModbusASCIISlave)
	{
		for( ; ; )
		{

			//we're starting high.  (we'll assume that we're not in the middle of a byte. 
			mModbus->AdvanceToNextEdge();

			//we're now at the beginning of the start bit.  We can start collecting the data.
			U64 frame_starting_sample = mModbus->GetSampleNumber();

			U64 data = 0;
			bool parity_error = false;
			bool framing_error = false;
			bool mp_is_address = false;
			
			DataBuilder data_builder;
			data_builder.Reset( &data, mSettings->mShiftOrder, num_bits );
			U64 marker_location = frame_starting_sample;

			for( U32 i=0; i<num_bits; i++ )
			{
				mModbus->Advance( mSampleOffsets[i] );
				data_builder.AddBit( mModbus->GetBitState() );

				marker_location += mSampleOffsets[i];
				mResults->AddMarker( marker_location, AnalyzerResults::Dot, mSettings->mInputChannel );
			}

			if( mSettings->mInverted == true )
				data = (~data) & bit_mask;

			if( mSettings->mModbusMode != ModbusAnalyzerEnums::Normal )
			{
				//extract the MSB
				U64 msb = data >> (num_bits - 1);
				msb &= 0x1;
				if( mSettings->mModbusMode == ModbusAnalyzerEnums::MpModeMsbOneMeansAddress )
				{
					if( msb == 0x0 )
						mp_is_address = false;
					else
						mp_is_address = true;
				}
				if( mSettings->mModbusMode == ModbusAnalyzerEnums::MpModeMsbZeroMeansAddress )
				{
					if( msb == 0x0 )
						mp_is_address = true;
					else
						mp_is_address = false;
				}
				//now remove the msb.
				data &= ( bit_mask >> 1 );
			}
				
			parity_error = false;

			if( mSettings->mParity != AnalyzerEnums::None )
			{
				mModbus->Advance( mParityBitOffset );
				bool is_even = AnalyzerHelpers::IsEven( AnalyzerHelpers::GetOnesCount( data ) );

				if( mSettings->mParity == AnalyzerEnums::Even )
				{
					if( is_even == true )
					{
						if( mModbus->GetBitState() != mBitLow ) //we expect a low bit, to keep the parity even.
							parity_error = true;
					}else
					{
						if( mModbus->GetBitState() != mBitHigh ) //we expect a high bit, to force parity even.
							parity_error = true;
					}
				}else  //if( mSettings->mParity == AnalyzerEnums::Odd )
				{
					if( is_even == false )
					{
						if( mModbus->GetBitState() != mBitLow ) //we expect a low bit, to keep the parity odd.
							parity_error = true;
					}else
					{
						if( mModbus->GetBitState() != mBitHigh ) //we expect a high bit, to force parity odd.
							parity_error = true;
					}
				}

				marker_location += mParityBitOffset;
				mResults->AddMarker( marker_location, AnalyzerResults::Square, mSettings->mInputChannel );
			}

			//now we must dermine if there is a framing error.
			framing_error = false;

			mModbus->Advance( mStartOfStopBitOffset );

			if( mModbus->GetBitState() != mBitHigh )
			{
				framing_error = true;
			}else
			{
				U32 num_edges = mModbus->Advance( mEndOfStopBitOffset );
				if( num_edges != 0 )
					framing_error = true;
			}

			if( framing_error == true )
			{
				marker_location += mStartOfStopBitOffset;
				mResults->AddMarker( marker_location, AnalyzerResults::ErrorX, mSettings->mInputChannel );

				if( mEndOfStopBitOffset != 0 )
				{
					marker_location += mEndOfStopBitOffset;
					mResults->AddMarker( marker_location, AnalyzerResults::ErrorX, mSettings->mInputChannel );
				}
			}

			//ok now record the value!
			//note that we're not using the mData2 or mType fields for anything, so we won't bother to set them.
			Frame frame;
			frame.mStartingSampleInclusive = frame_starting_sample;
			frame.mEndingSampleInclusive = mModbus->GetSampleNumber();
			frame.mData1 = data;
			frame.mFlags = 0;
			if( parity_error == true )
				frame.mFlags |= PARITY_ERROR_FLAG | DISPLAY_AS_ERROR_FLAG;

			if( framing_error == true )
				frame.mFlags |= FRAMING_ERROR_FLAG | DISPLAY_AS_ERROR_FLAG;

			if( mp_is_address == true )
				frame.mFlags |= MP_MODE_ADDRESS_FLAG;

			if( mp_is_address == true )
				mResults->CommitPacketAndStartNewPacket();

			mResults->AddFrame( frame );

			mResults->CommitResults();

			ReportProgress( frame.mEndingSampleInclusive );
			CheckIfThreadShouldExit();

			if( framing_error == true )  //if we're still low, let's fix that for the next round.
			{
				if( mModbus->GetBitState() == mBitLow )
					mModbus->AdvanceToNextEdge();
			}
		}
	}
	//Analyze using Modbus extension
	else if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusASCIIMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusASCIISlave)
	{
		for( ; ; )
		{

			Frame frame;
			U64 starting_frame;
			U64 ending_frame;
			U64 Payload1[2];
			U64 Payload2[2];
			U64 Payload3[2];
			U64 Payload4[2];
			U64 RecChecksum[2];
			U64 ByteCount[2];
			U64 Checksum; 
			
			//if analyzer is in ASCII mode, we need to make sure we catch the ':' byte as the start of frame, RTU just uses silence
			if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusASCIIMaster || mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusASCIISlave)
			{
				char rawdata = 0x00;
				do{
					rawdata = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					frame.mStartingSampleInclusive = starting_frame;
				}while(rawdata!=':');
			}
			
			//the frame begins here with the Device Address

			U64 devaddr = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
			frame.mStartingSampleInclusive = starting_frame;
			
			//Then comes the Function Code
			U64 funccode = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
			
			//Now we'll process the rest of the data based on whether the transmission is coming from the master or a slave device
			if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusASCIIMaster)
			{
				frame.mFlags = FLAG_REQUEST_FRAME;

				//It's a master device doing the talking
				switch(funccode)
				{
					//use fall through to process similar requests with the same code
				case FUNCCODE_READ_COILS: 
				case FUNCCODE_READ_DISCRETE_INPUTS:
				case FUNCCODE_READ_HOLDING_REGISTERS:
				case FUNCCODE_READ_INPUT_REGISTER:
				case FUNCCODE_WRITE_SINGLE_COIL:
				case FUNCCODE_WRITE_SINGLE_REGISTER:
				case FUNCCODE_DIAGNOSTIC:

					Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster)
					{
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, Payload1[0] );
						Checksum = update_CRC( Checksum, Payload1[1] );
						Checksum = update_CRC( Checksum, Payload2[0] );
						Checksum = update_CRC( Checksum, Payload2[1] );

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
					}
					else
					{
						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
				
						Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + Payload1[0];
						Checksum = Checksum + Payload1[1];
						Checksum = Checksum + Payload2[0];
						Checksum = Checksum + Payload2[1];

						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
					}
					break;

					//this set of cases have requests which are just 1 byte long (function code only)
				case FUNCCODE_READ_EXCEPTION_STATUS:
				case FUNCCODE_GET_COM_EVENT_COUNTER:
				case FUNCCODE_GET_COM_EVENT_LOG:
				case FUNCCODE_REPORT_SLAVE_ID:

					Payload1[0] = 0x00;
					Payload1[1] = 0x00;

					Payload2[0] = 0x00;
					Payload2[1] = 0x00;

					if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster)
					{
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
					}
					else
					{
						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
				
						Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;

						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
					}
					break;
				case FUNCCODE_WRITE_MULTIPLE_COILS:
					
					Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					ByteCount[1] = 0x00;

					if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster)
					{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, Payload1[0] );
						Checksum = update_CRC( Checksum, Payload1[1] );
						Checksum = update_CRC( Checksum, Payload2[0] );
						Checksum = update_CRC( Checksum, Payload2[1] );
						Checksum = update_CRC( Checksum, ByteCount[0] );

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0];i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload1[1] = 0x00;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							DataFrame.mStartingSampleInclusive = starting_frame;
							DataFrame.mEndingSampleInclusive = ending_frame;

							Checksum = update_CRC( Checksum, Payload1[0] );

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						//end this frame here and make frames for each of the output values
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						
					}
					else
					{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();
				
						Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + Payload1[0];
						Checksum = Checksum + Payload1[1];
						Checksum = Checksum + Payload2[0];
						Checksum = Checksum + Payload2[1];
						Checksum = Checksum + ByteCount[0];

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0];i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload1[1] = 0x00;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							DataFrame.mStartingSampleInclusive = starting_frame;
							DataFrame.mEndingSampleInclusive = ending_frame;

							Checksum = Checksum + Payload1[0];

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
					}
					break;
				case FUNCCODE_WRITE_MULTIPLE_REGISTERS:
					
					Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					ByteCount[1] = 0x00;

					if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster)
					{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, Payload1[0] );
						Checksum = update_CRC( Checksum, Payload1[1] );
						Checksum = update_CRC( Checksum, Payload2[0] );
						Checksum = update_CRC( Checksum, Payload2[1] );
						Checksum = update_CRC( Checksum, ByteCount[0] );

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0]/2;i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							Checksum = update_CRC( Checksum, Payload1[0] );
							Checksum = update_CRC( Checksum, Payload1[1] );

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}

						//end this frame here and make frames for each of the output values
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						
					}
					else
					{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();
				
						Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + Payload1[0];
						Checksum = Checksum + Payload1[1];
						Checksum = Checksum + Payload2[0];
						Checksum = Checksum + Payload2[1];
						Checksum = Checksum + ByteCount[0];

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0]/2;i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[0] <<40) + (Payload1[1] << 32);
							Checksum = Checksum + Payload1[0];
							Checksum = Checksum + Payload1[1];

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
					}
					break;
				case FUNCCODE_READ_FILE_RECORD:
					
					Payload1[0] = 0x00;
					Payload1[1] = 0x00;

					Payload2[0] = 0x00;
					Payload2[1] = 0x00;

					ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					ByteCount[1] = 0x00;

					if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster)
					{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, ByteCount[0] );

						Frame DataFrame;
						
						for(int i=0;i<((ByteCount[1]<<8)+ByteCount[0]);i=i+7)
						{
							DataFrame.mFlags = FLAG_FILE_SUBREQ;
							
							//Reference Type
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;
			
							Payload1[1] = 0x00;
							
							//File number
							Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

							//Record Number
							Payload3[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload3[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							
							//Record Length
							Payload4[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload4[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] << 56) + (Payload1[0] << 48) + (Payload2[1] <<40) + (Payload2[0] << 32) + (Payload3[1] << 24) + (Payload3[0]<<16) + (Payload4[1] << 8) + Payload4[0];
							Checksum = update_CRC( Checksum, Payload1[0] );
							Checksum = update_CRC( Checksum, Payload2[0] );
							Checksum = update_CRC( Checksum, Payload2[1] );
							Checksum = update_CRC( Checksum, Payload3[0] );
							Checksum = update_CRC( Checksum, Payload3[1] );
							Checksum = update_CRC( Checksum, Payload4[0] );
							Checksum = update_CRC( Checksum, Payload4[1] );

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						//end this frame here and make frames for each of the output values
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						
					}
					else
					{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();
				
						Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + ByteCount[0];

						Frame DataFrame;
						
						for(int i=0;i<((ByteCount[1]<<8)+ByteCount[0]);i=i+7)
						{
							DataFrame.mFlags = FLAG_FILE_SUBREQ;
							
							//Reference Type
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;
			
							Payload1[1] = 0x00;
							
							//File number
							Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

							//Record Number
							Payload3[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload3[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							
							//Record Length
							Payload4[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload4[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] << 56) + (Payload1[0] << 48) + (Payload2[0] <<40) + (Payload2[1] << 32) + (Payload3[0] << 24) + (Payload3[1]<<16) + (Payload4[0] << 8) + Payload4[1];
							Checksum = Checksum + Payload1[0];
							Checksum = Checksum + Payload2[0];
							Checksum = Checksum + Payload2[1];
							Checksum = Checksum + Payload3[0];
							Checksum = Checksum + Payload3[1];
							Checksum = Checksum + Payload4[0];
							Checksum = Checksum + Payload4[1];

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
					}
					break;
				case FUNCCODE_WRITE_FILE_RECORD:
					Payload1[0] = 0x00;
					Payload1[1] = 0x00;

					Payload2[0] = 0x00;
					Payload2[1] = 0x00;

					ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					ByteCount[1] = 0x00;

					if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster)
					{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, ByteCount[0] );

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0];i=i+9)
						{
							DataFrame.mFlags = FLAG_FILE_SUBREQ;
							
							//Reference Type
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;
			
							Payload1[1] = 0x00;
							
							//File number
							Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

							//Record Number
							Payload3[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload3[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							
							//Record Length
							Payload4[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload4[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] << 56) + (Payload1[0] << 48) + (Payload2[1] <<40) + (Payload2[0] << 32) + (Payload3[1] << 24) + (Payload3[0]<<16) + (Payload4[1] << 8) + Payload4[0];
							Checksum = update_CRC( Checksum, Payload1[0] );
							Checksum = update_CRC( Checksum, Payload2[0] );
							Checksum = update_CRC( Checksum, Payload2[1] );
							Checksum = update_CRC( Checksum, Payload3[0] );
							Checksum = update_CRC( Checksum, Payload3[1] );
							Checksum = update_CRC( Checksum, Payload4[0] );
							Checksum = update_CRC( Checksum, Payload4[1] );

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();

							Frame RecDataFrame;
							int k;
							for(k=0;k<((Payload4[1]<<8)+Payload4[0]);k++)
							{
								RecDataFrame.mFlags = FLAG_DATA_FRAME;
								
								Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								RecDataFrame.mStartingSampleInclusive = starting_frame;
								Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

								RecDataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
								RecDataFrame.mEndingSampleInclusive = ending_frame;

								Checksum = update_CRC( Checksum, Payload1[0] );
								Checksum = update_CRC( Checksum, Payload1[1] );

								mResults->AddFrame( RecDataFrame );
								mResults->CommitResults();
							}
							i=i+(k*2);
						}
						
						//end this frame here and make frames for each of the output values
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						
					}
					else
					{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();
				
						Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + ByteCount[0];

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0];i=i+9)
						{
							DataFrame.mFlags = FLAG_FILE_SUBREQ;
							
							//Reference Type
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;
			
							Payload1[1] = 0x00;
							
							//File number
							Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

							//Record Number
							Payload3[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload3[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							
							//Record Length
							Payload4[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							Payload4[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] << 56) + (Payload1[0] << 48) + (Payload2[0] <<40) + (Payload2[1] << 32) + (Payload3[0] << 24) + (Payload3[1]<<16) + (Payload4[0] << 8) + Payload4[1];

							Checksum = Checksum + Payload1[0];
							Checksum = Checksum + Payload2[0];
							Checksum = Checksum + Payload2[1];
							Checksum = Checksum + Payload3[0];
							Checksum = Checksum + Payload3[1];
							Checksum = Checksum + Payload4[0];
							Checksum = Checksum + Payload4[1];

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();

							Frame RecDataFrame;
							int k;
							for(k=0;k<((Payload4[0]<<8)+Payload4[1]);k++)
							{
								RecDataFrame.mFlags = FLAG_DATA_FRAME;
								
								Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								RecDataFrame.mStartingSampleInclusive = starting_frame;
								Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

								RecDataFrame.mData1 = (Payload1[0] <<40) + (Payload1[1] << 32);
								RecDataFrame.mEndingSampleInclusive = ending_frame;

								Checksum = Checksum + Payload1[0];
								Checksum = Checksum + Payload1[1];

								mResults->AddFrame( RecDataFrame );
								mResults->CommitResults();
							}
							i=i+(k*2);
						}
						
						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
					}
					break;
				case FUNCCODE_MASK_WRITE_REGISTER:

					Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					
					Payload3[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload3[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster)
					{
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, Payload1[0] );
						Checksum = update_CRC( Checksum, Payload1[1] );
						Checksum = update_CRC( Checksum, Payload2[0] );
						Checksum = update_CRC( Checksum, Payload2[1] );
						Checksum = update_CRC( Checksum, Payload3[0] );
						Checksum = update_CRC( Checksum, Payload3[1] );

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
						frame.mData2 = (Payload3[1]<<8) + Payload3[0];
					}
					else
					{
						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
				
						Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + Payload1[0];
						Checksum = Checksum + Payload1[1];
						Checksum = Checksum + Payload2[0];
						Checksum = Checksum + Payload2[1];
						Checksum = Checksum + Payload3[0];
						Checksum = Checksum + Payload3[1];

						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
						frame.mData2 = (Payload3[0]<<8) + Payload3[1];
					}
					break;
				case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
					//code this section and master is done.. wooo

					//Read Starting Address
					Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					//Quantity to read
					Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					//Write Starting Address
					Payload3[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload3[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					//Quantity to write
					Payload4[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload4[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					
					//Write Byte Count
					ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					ByteCount[1] = 0x00;

					if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster)
					{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mData2 = (Payload3[1] << 24) + (Payload3[0]<<16) + (Payload4[1] << 8) + Payload4[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, Payload1[0] );
						Checksum = update_CRC( Checksum, Payload1[1] );
						Checksum = update_CRC( Checksum, Payload2[0] );
						Checksum = update_CRC( Checksum, Payload2[1] );
						Checksum = update_CRC( Checksum, Payload3[0] );
						Checksum = update_CRC( Checksum, Payload3[1] );
						Checksum = update_CRC( Checksum, Payload4[0] );
						Checksum = update_CRC( Checksum, Payload4[1] );
						Checksum = update_CRC( Checksum, ByteCount[0] );

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0]/2;i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							Checksum = update_CRC( Checksum, Payload1[0] );
							Checksum = update_CRC( Checksum, Payload1[1] );

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}

						//end this frame here and make frames for each of the output values
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						
					}
					else
					{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mData2 = (Payload3[0] << 24) + (Payload3[1]<<16) + (Payload4[0] << 8) + Payload4[1];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();
				
						Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + Payload1[0];
						Checksum = Checksum + Payload1[1];
						Checksum = Checksum + Payload2[0];
						Checksum = Checksum + Payload2[1];
						Checksum = Checksum + Payload3[0];
						Checksum = Checksum + Payload3[1];
						Checksum = Checksum + Payload4[0];
						Checksum = Checksum + Payload4[1];
						Checksum = Checksum + ByteCount[0];

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0]/2;i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[0] <<40) + (Payload1[1] << 32);
							Checksum = Checksum + Payload1[0];
							Checksum = Checksum + Payload1[1];

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
					}
					break;

				case FUNCCODE_READ_FIFO_QUEUE:
					Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

					Payload2[0] = 0x00;
					Payload2[1] = 0x00;

					if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUMaster)
					{
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, Payload1[0] );
						Checksum = update_CRC( Checksum, Payload1[1] );

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
					}
					else
					{
						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
				
						Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + Payload1[0];
						Checksum = Checksum + Payload1[1];

						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
					}
					break;

				default:
					break;
				}
			}
			else
			{
				//slave mode
				//check if it's a normal response or a Nak/Error
				if(funccode&0x80)
				{
					//it's a NAK/Error
					frame.mFlags = FLAG_EXCEPTION_FRAME;

						Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload1[1] = 0x00;

						Payload2[0] = 0x00;
						Payload2[1] = 0x00;

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

							Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
							Checksum = update_CRC( Checksum, devaddr ); 
							Checksum = update_CRC( Checksum, funccode );
							Checksum = update_CRC( Checksum, Payload1[0] );

							if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
							}
							
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
						}
						else
						{
							RecChecksum[1] = 0x00;
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

							Checksum = Checksum + devaddr;
							Checksum = Checksum + funccode;
							Checksum = Checksum + Payload1[0];

							Checksum =  ~Checksum +1 ;
							Checksum = Checksum &0x00FF;

							if(Checksum!=RecChecksum[0])
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
							}

							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
						}

				}
				else
				{
					//it's a valid response
					frame.mFlags = FLAG_RESPONSE_FRAME;

					//It's a master device doing the talking
					switch(funccode)
					{
						//use fall through to process similar requests with the same code
					case FUNCCODE_READ_COILS: 
					case FUNCCODE_READ_DISCRETE_INPUTS:

						Payload1[0] = 0x00;
						Payload1[1] = 0x00;

						Payload2[0] = 0x00;
						Payload2[1] = 0x00;

						ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						ByteCount[1] = 0x00;

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, ByteCount[0] );

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0];i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = 0x00;
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							Checksum = update_CRC( Checksum, Payload1[0] );

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}

						//end this frame here and make frames for each of the output values
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						else
						{
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
							frame.mEndingSampleInclusive = ending_frame;
							mResults->AddFrame( frame );
							mResults->CommitResults();
				
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + ByteCount[0];

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0];i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = 0x00;
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							Checksum = Checksum + Payload1[0];

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						break;

					case FUNCCODE_READ_HOLDING_REGISTERS:
					case FUNCCODE_READ_INPUT_REGISTER:
					case FUNCCODE_READWRITE_MULTIPLE_REGISTERS:
						
						Payload1[0] = 0x00;
						Payload1[1] = 0x00;

						Payload2[0] = 0x00;
						Payload2[1] = 0x00;

						ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						ByteCount[1] = 0x00;

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, ByteCount[0] );

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0]/2;i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							Checksum = update_CRC( Checksum, Payload1[0] );
							Checksum = update_CRC( Checksum, Payload1[1] );

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}

						//end this frame here and make frames for each of the output values
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						else
						{
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
							frame.mEndingSampleInclusive = ending_frame;
							mResults->AddFrame( frame );
							mResults->CommitResults();
				
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + ByteCount[0];

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0]/2;i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[0] <<40) + (Payload1[1] << 32);
							Checksum = Checksum + Payload1[0];
							Checksum = Checksum + Payload1[1];

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						break;
					
					case FUNCCODE_WRITE_SINGLE_COIL:
					case FUNCCODE_WRITE_SINGLE_REGISTER:
					case FUNCCODE_DIAGNOSTIC:
					case FUNCCODE_GET_COM_EVENT_COUNTER:
					case FUNCCODE_WRITE_MULTIPLE_COILS:
					case FUNCCODE_WRITE_MULTIPLE_REGISTERS:

						Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

							Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
							Checksum = update_CRC( Checksum, devaddr ); 
							Checksum = update_CRC( Checksum, funccode );
							Checksum = update_CRC( Checksum, Payload1[0] );
							Checksum = update_CRC( Checksum, Payload1[1] );
							Checksum = update_CRC( Checksum, Payload2[0] );
							Checksum = update_CRC( Checksum, Payload2[1] );

							if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
							}
							
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
						}
						else
						{
							RecChecksum[1] = 0x00;
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

							Checksum = Checksum + devaddr;
							Checksum = Checksum + funccode;
							Checksum = Checksum + Payload1[0];
							Checksum = Checksum + Payload1[1];
							Checksum = Checksum + Payload2[0];
							Checksum = Checksum + Payload2[1];

							Checksum =  ~Checksum +1 ;
							Checksum = Checksum &0x00FF;

							if(Checksum!=RecChecksum[0])
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
							}

							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
						}
						break;

					case FUNCCODE_READ_EXCEPTION_STATUS:
						Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload1[1] = 0x00;

						Payload2[0] = 0x00;
						Payload2[1] = 0x00;

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

							Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
							Checksum = update_CRC( Checksum, devaddr ); 
							Checksum = update_CRC( Checksum, funccode );
							Checksum = update_CRC( Checksum, Payload1[0] );

							if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
							}
							
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
						}
						else
						{
							RecChecksum[1] = 0x00;
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

							Checksum = Checksum + devaddr;
							Checksum = Checksum + funccode;
							Checksum = Checksum + Payload1[0];

							Checksum =  ~Checksum +1 ;
							Checksum = Checksum &0x00FF;

							if(Checksum!=RecChecksum[0])
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
							}

							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
						}
						break;
					case FUNCCODE_REPORT_SLAVE_ID:
						
						Payload1[0] = 0x00;
						Payload1[1] = 0x00;

						Payload2[0] = 0x00;
						Payload2[1] = 0x00;

						ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						ByteCount[1] = 0x00;

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, ByteCount[0] );

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0];i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = 0x00;
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							Checksum = update_CRC( Checksum, Payload1[0] );

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}

						//end this frame here and make frames for each of the output values
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						else
						{
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
							frame.mEndingSampleInclusive = ending_frame;
							mResults->AddFrame( frame );
							mResults->CommitResults();
				
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + ByteCount[0];

						Frame DataFrame;
						
						for(int i=0;i<ByteCount[0];i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = 0x00;
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							Checksum = Checksum + Payload1[0];

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

							frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						break;

					case FUNCCODE_GET_COM_EVENT_LOG:

						ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						ByteCount[1] = 0x00;

						Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						Payload2[0] = 0x00;
						Payload2[1] = 0x00;

						Payload3[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload3[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						Payload4[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload4[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mData2 = (Payload3[1] << 24) + (Payload3[0]<<16) + (Payload4[1] << 8) + Payload4[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, ByteCount[0] );
						Checksum = update_CRC( Checksum, Payload1[0] );
						Checksum = update_CRC( Checksum, Payload1[1] );
						Checksum = update_CRC( Checksum, Payload3[0] );
						Checksum = update_CRC( Checksum, Payload3[1] );
						Checksum = update_CRC( Checksum, Payload4[0] );
						Checksum = update_CRC( Checksum, Payload4[1] );

						Frame DataFrame;
						
						for(int i=0;i<(ByteCount[0]-6);i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = 0x00;
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							Checksum = update_CRC( Checksum, Payload1[0] );

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}

						//end this frame here and make frames for each of the output values
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						else
						{
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
							frame.mData2 = (Payload3[0] << 24) + (Payload3[1]<<16) + (Payload4[0] << 8) + Payload4[1];
							frame.mEndingSampleInclusive = ending_frame;
							mResults->AddFrame( frame );
							mResults->CommitResults();
				
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + ByteCount[0];
						Checksum = Checksum + Payload1[0];
						Checksum = Checksum + Payload1[1];
						Checksum = Checksum + Payload3[0];
						Checksum = Checksum + Payload3[1];
						Checksum = Checksum + Payload4[0];
						Checksum = Checksum + Payload4[1];

						Frame DataFrame;
						
						for(int i=0;i<(ByteCount[0]-6);i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = 0x00;
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							Checksum = Checksum + Payload1[0];

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						break;

					case FUNCCODE_READ_FILE_RECORD:
						Payload1[0] = 0x00;
						Payload1[1] = 0x00;

						Payload2[0] = 0x00;
						Payload2[1] = 0x00;

						ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						ByteCount[1] = 0x00;

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
							frame.mEndingSampleInclusive = ending_frame;
							mResults->AddFrame( frame );
							mResults->CommitResults();

							Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
							Checksum = update_CRC( Checksum, devaddr ); 
							Checksum = update_CRC( Checksum, funccode );
							Checksum = update_CRC( Checksum, ByteCount[0] );

							Frame DataFrame;
							
							for(int i=0;i<ByteCount[0];i=i+4)
							{
								DataFrame.mFlags = FLAG_FILE_SUBREQ;
								
								//Record Length
								Payload4[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								DataFrame.mStartingSampleInclusive = starting_frame;
								Payload4[1]= 0x00;

								//Reference Type
								Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								Payload1[1] = 0x00;
								
								//File number
								Payload2[0] = 0x00;
								Payload2[1] = 0x00;

								//Record Number
								Payload3[0]= 0x00;
								Payload3[1]= 0x00;
								
								DataFrame.mEndingSampleInclusive = ending_frame;

								DataFrame.mData1 = (Payload1[1] << 56) + (Payload1[0] << 48) + (Payload2[1] <<40) + (Payload2[0] << 32) + (Payload3[1] << 24) + (Payload3[0]<<16) + (Payload4[1] << 8) + Payload4[0];
								Checksum = update_CRC( Checksum, Payload4[0] );
								Checksum = update_CRC( Checksum, Payload1[0] );

								mResults->AddFrame( DataFrame );
								mResults->CommitResults();

								Frame RecDataFrame;
								int k;
								for(k=0;k<(Payload4[0]-1);k=k+2)
								{
									RecDataFrame.mFlags = FLAG_DATA_FRAME;
									
									Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
									RecDataFrame.mStartingSampleInclusive = starting_frame;
									Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

									RecDataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
									RecDataFrame.mEndingSampleInclusive = ending_frame;

									Checksum = update_CRC( Checksum, Payload1[0] );
									Checksum = update_CRC( Checksum, Payload1[1] );

									mResults->AddFrame( RecDataFrame );
									mResults->CommitResults();
								}
								i=i+k;
							}
							
							//end this frame here and make frames for each of the output values
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							frame.mFlags = FLAG_END_FRAME;
							frame.mStartingSampleInclusive = starting_frame;

							RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

							if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
							}
							
							frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
							
						}
						else
						{
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
							frame.mEndingSampleInclusive = ending_frame;
							mResults->AddFrame( frame );
							mResults->CommitResults();
					
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

							Checksum = Checksum + devaddr;
							Checksum = Checksum + funccode;
							Checksum = Checksum + ByteCount[0];

							Frame DataFrame;
							
							for(int i=0;i<ByteCount[0];i=i+2)
							{
								DataFrame.mFlags = FLAG_FILE_SUBREQ;
								
								//Record Length
								Payload4[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								DataFrame.mStartingSampleInclusive = starting_frame;
								Payload4[1]= 0x00;

								//Reference Type
								Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								Payload1[1] = 0x00;							
								DataFrame.mEndingSampleInclusive = ending_frame;

								Payload2[0] = 0x00;
								Payload2[1] = 0x00;

								Payload3[0] = 0x00;
								Payload3[1] = 0x00;

								DataFrame.mData1 = (Payload1[1] << 56) + (Payload1[0] << 48) + (Payload2[0] <<40) + (Payload2[1] << 32) + (Payload3[0] << 24) + (Payload3[1]<<16) + (Payload4[1] << 8) + Payload4[0];

								Checksum = Checksum + Payload4[0];
								Checksum = Checksum + Payload1[0];

								mResults->AddFrame( DataFrame );
								mResults->CommitResults();

								Frame RecDataFrame;
								int k;
								for(k=0;k<(Payload4[0]-1);k=k+2)
								{
									RecDataFrame.mFlags = FLAG_DATA_FRAME;
									
									Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
									RecDataFrame.mStartingSampleInclusive = starting_frame;
									Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

									RecDataFrame.mData1 = (Payload1[0] <<40) + (Payload1[1] << 32);
									RecDataFrame.mEndingSampleInclusive = ending_frame;

									Checksum = Checksum + Payload1[0];
									Checksum = Checksum + Payload1[1];

									mResults->AddFrame( RecDataFrame );
									mResults->CommitResults();
								}
								i=i+k;
							}
							
							Checksum =  ~Checksum +1 ;
							Checksum = Checksum &0x00FF;

							RecChecksum[1] = 0x00;
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							frame.mFlags = FLAG_END_FRAME;
							frame.mStartingSampleInclusive = starting_frame;

							if(Checksum!=RecChecksum[0])
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
							}

							frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						break;

					case FUNCCODE_WRITE_FILE_RECORD:
						Payload1[0] = 0x00;
						Payload1[1] = 0x00;

						Payload2[0] = 0x00;
						Payload2[1] = 0x00;

						ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						ByteCount[1] = 0x00;

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
							frame.mEndingSampleInclusive = ending_frame;
							mResults->AddFrame( frame );
							mResults->CommitResults();

							Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
							Checksum = update_CRC( Checksum, devaddr ); 
							Checksum = update_CRC( Checksum, funccode );
							Checksum = update_CRC( Checksum, ByteCount[0] );

							Frame DataFrame;
							
							for(int i=0;i<ByteCount[0];i=i+9)
							{
								DataFrame.mFlags = FLAG_FILE_SUBREQ;
								
								//Reference Type
								Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								DataFrame.mStartingSampleInclusive = starting_frame;
				
								Payload1[1] = 0x00;
								
								//File number
								Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

								//Record Number
								Payload3[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								Payload3[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								
								//Record Length
								Payload4[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								Payload4[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								
								DataFrame.mEndingSampleInclusive = ending_frame;

								DataFrame.mData1 = (Payload1[1] << 56) + (Payload1[0] << 48) + (Payload2[1] <<40) + (Payload2[0] << 32) + (Payload3[1] << 24) + (Payload3[0]<<16) + (Payload4[1] << 8) + Payload4[0];
								Checksum = update_CRC( Checksum, Payload1[0] );
								Checksum = update_CRC( Checksum, Payload2[0] );
								Checksum = update_CRC( Checksum, Payload2[1] );
								Checksum = update_CRC( Checksum, Payload3[0] );
								Checksum = update_CRC( Checksum, Payload3[1] );
								Checksum = update_CRC( Checksum, Payload4[0] );
								Checksum = update_CRC( Checksum, Payload4[1] );

								mResults->AddFrame( DataFrame );
								mResults->CommitResults();

								Frame RecDataFrame;
								int k;
								for(k=0;k<((Payload4[1]<<8)+Payload4[0]);k++)
								{
									RecDataFrame.mFlags = FLAG_DATA_FRAME;
									
									Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
									RecDataFrame.mStartingSampleInclusive = starting_frame;
									Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

									RecDataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
									RecDataFrame.mEndingSampleInclusive = ending_frame;

									Checksum = update_CRC( Checksum, Payload1[0] );
									Checksum = update_CRC( Checksum, Payload1[1] );

									mResults->AddFrame( RecDataFrame );
									mResults->CommitResults();
								}
								i=i+(k*2);
							}
							
							//end this frame here and make frames for each of the output values
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							frame.mFlags = FLAG_END_FRAME;
							frame.mStartingSampleInclusive = starting_frame;

							RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

							if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
							}
							
							frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
							
						}
						else
						{
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[1] << 8) + ByteCount[0];
							frame.mEndingSampleInclusive = ending_frame;
							mResults->AddFrame( frame );
							mResults->CommitResults();
					
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

							Checksum = Checksum + devaddr;
							Checksum = Checksum + funccode;
							Checksum = Checksum + ByteCount[0];

							Frame DataFrame;
							
							for(int i=0;i<ByteCount[0];i=i+9)
							{
								DataFrame.mFlags = FLAG_FILE_SUBREQ;
								
								//Reference Type
								Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								DataFrame.mStartingSampleInclusive = starting_frame;
				
								Payload1[1] = 0x00;
								
								//File number
								Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

								//Record Number
								Payload3[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								Payload3[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								
								//Record Length
								Payload4[0]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								Payload4[1]= GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
								
								DataFrame.mEndingSampleInclusive = ending_frame;

								DataFrame.mData1 = (Payload1[1] << 56) + (Payload1[0] << 48) + (Payload2[0] <<40) + (Payload2[1] << 32) + (Payload3[0] << 24) + (Payload3[1]<<16) + (Payload4[0] << 8) + Payload4[1];

								Checksum = Checksum + Payload1[0];
								Checksum = Checksum + Payload2[0];
								Checksum = Checksum + Payload2[1];
								Checksum = Checksum + Payload3[0];
								Checksum = Checksum + Payload3[1];
								Checksum = Checksum + Payload4[0];
								Checksum = Checksum + Payload4[1];

								mResults->AddFrame( DataFrame );
								mResults->CommitResults();

								Frame RecDataFrame;
								int k;
								for(k=0;k<((Payload4[0]<<8)+Payload4[1]);k++)
								{
									RecDataFrame.mFlags = FLAG_DATA_FRAME;
									
									Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
									RecDataFrame.mStartingSampleInclusive = starting_frame;
									Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

									RecDataFrame.mData1 = (Payload1[0] <<40) + (Payload1[1] << 32);
									RecDataFrame.mEndingSampleInclusive = ending_frame;

									Checksum = Checksum + Payload1[0];
									Checksum = Checksum + Payload1[1];

									mResults->AddFrame( RecDataFrame );
									mResults->CommitResults();
								}
								i=i+(k*2);
							}
							
							Checksum =  ~Checksum +1 ;
							Checksum = Checksum &0x00FF;

							RecChecksum[1] = 0x00;
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							frame.mFlags = FLAG_END_FRAME;
							frame.mStartingSampleInclusive = starting_frame;

							if(Checksum!=RecChecksum[0])
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
							}

							frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						break;

					case FUNCCODE_MASK_WRITE_REGISTER:
						
						Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						
						Payload3[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload3[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

							Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
							Checksum = update_CRC( Checksum, devaddr ); 
							Checksum = update_CRC( Checksum, funccode );
							Checksum = update_CRC( Checksum, Payload1[0] );
							Checksum = update_CRC( Checksum, Payload1[1] );
							Checksum = update_CRC( Checksum, Payload2[0] );
							Checksum = update_CRC( Checksum, Payload2[1] );
							Checksum = update_CRC( Checksum, Payload3[0] );
							Checksum = update_CRC( Checksum, Payload3[1] );

							if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
							}
							
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
							frame.mData2 = (Payload3[1]<<8) + Payload3[0];
						}
						else
						{
							RecChecksum[1] = 0x00;
							RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
					
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

							Checksum = Checksum + devaddr;
							Checksum = Checksum + funccode;
							Checksum = Checksum + Payload1[0];
							Checksum = Checksum + Payload1[1];
							Checksum = Checksum + Payload2[0];
							Checksum = Checksum + Payload2[1];
							Checksum = Checksum + Payload3[0];
							Checksum = Checksum + Payload3[1];

							Checksum =  ~Checksum +1 ;
							Checksum = Checksum &0x00FF;

							if(Checksum!=RecChecksum[0])
							{
								frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
							}

							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (RecChecksum[1] << 8) + RecChecksum[0];
							frame.mData2 = (Payload3[0]<<8) + Payload3[1];
						}
						break;

					case FUNCCODE_READ_FIFO_QUEUE:

						ByteCount[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						ByteCount[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						Payload2[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						Payload2[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						Payload1[0] = 0x00;
						Payload1[1] = 0x00;

						if(mSettings->mModbusMode==ModbusAnalyzerEnums::ModbusRTUSlave)
						{
						frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[1] <<40) + (Payload1[0] << 32) + (Payload2[1] << 24) + (Payload2[0]<<16) + (ByteCount[1] << 8) + ByteCount[0];
						frame.mEndingSampleInclusive = ending_frame;
						mResults->AddFrame( frame );
						mResults->CommitResults();

						Checksum = 0xFFFF; //Modbus/RTU uses CRC-16, calls for initialization to 0xFFFF
						Checksum = update_CRC( Checksum, devaddr ); 
						Checksum = update_CRC( Checksum, funccode );
						Checksum = update_CRC( Checksum, ByteCount[0] );
						Checksum = update_CRC( Checksum, ByteCount[1] );
						Checksum = update_CRC( Checksum, Payload2[0] );
						Checksum = update_CRC( Checksum, Payload2[1] );

						Frame DataFrame;
						
						for(int i=0;i<((Payload2[1]<<8)+Payload2[0]);i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[1] <<40) + (Payload1[0] << 32);
							Checksum = update_CRC( Checksum, Payload1[0] );
							Checksum = update_CRC( Checksum, Payload1[1] );

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}

						//end this frame here and make frames for each of the output values
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						RecChecksum[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);

						if((((Checksum&0xFF00)>>8)!=RecChecksum[1]) || ((Checksum&0x00FF)!=RecChecksum[0]))
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//error!
						}
						
						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						else
						{
							frame.mData1 = (devaddr << 56) + (funccode << 48) + (Payload1[0] <<40) + (Payload1[1] << 32) + (Payload2[0] << 24) + (Payload2[1]<<16) + (ByteCount[0] << 8) + ByteCount[1];
							frame.mEndingSampleInclusive = ending_frame;
							mResults->AddFrame( frame );
							mResults->CommitResults();
				
							Checksum = 0x0000;   //Modbus/ASCII uses LRC, initialization to 0x0000;

						Checksum = Checksum + devaddr;
						Checksum = Checksum + funccode;
						Checksum = Checksum + ByteCount[0];
						Checksum = Checksum + ByteCount[1];
						Checksum = Checksum + Payload2[0];
						Checksum = Checksum + Payload2[1];

						Frame DataFrame;
						
						for(int i=0;i<((Payload2[0]<<8)+Payload2[1]);i++)
						{
							DataFrame.mFlags = FLAG_DATA_FRAME;
							
							Payload1[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mStartingSampleInclusive = starting_frame;

							Payload1[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
							DataFrame.mEndingSampleInclusive = ending_frame;

							DataFrame.mData1 = (Payload1[0] <<40) + (Payload1[1] << 32);
							Checksum = Checksum + Payload1[0];
							Checksum = Checksum + Payload1[1];

							mResults->AddFrame( DataFrame );
							mResults->CommitResults();
						}
						
						Checksum =  ~Checksum +1 ;
						Checksum = Checksum &0x00FF;

						RecChecksum[1] = 0x00;
						RecChecksum[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
						frame.mFlags = FLAG_END_FRAME;
						frame.mStartingSampleInclusive = starting_frame;

						if(Checksum!=RecChecksum[0])
						{
							frame.mFlags = frame.mFlags | FLAG_CHECKSUM_ERROR;	//Error with LRC error
						}

						frame.mData1 = (RecChecksum[1] << 8) + RecChecksum[0];
						}
						break;

					default:
						//nothing for now
						break;
					}

				}
			}

			//in ASCII mode, the frame ends with a \n \r termination, in RTU mode, just silence
			if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusASCIIMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusASCIISlave)
			{
				char StopFrame[2];
				StopFrame[0] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
				StopFrame[1] = GetNextByteModbus(num_bits, bit_mask, starting_frame, ending_frame);
			}

			//the frame ends here
			frame.mEndingSampleInclusive = ending_frame;
			mResults->AddFrame( frame );
			mResults->CommitResults();

			ReportProgress( frame.mEndingSampleInclusive );
			CheckIfThreadShouldExit();
		}
	}
}

bool ModbusAnalyzer::NeedsRerun()
{
	if( mSettings->mUseAutobaud == false )
		return false;

	//ok, lets see if we should change the bit rate, base on mShortestActivePulse

	U64 shortest_pulse = mModbus->GetMinimumPulseWidthSoFar();

	if( shortest_pulse == 0 )
		AnalyzerHelpers::Assert( "Alg problem, shortest_pulse was 0" );

	U32 computed_bit_rate = U32( double( mSampleRateHz ) / double( shortest_pulse ) );

	if( computed_bit_rate > mSampleRateHz )
		AnalyzerHelpers::Assert( "Alg problem, computed_bit_rate is higer than sample rate" );  //just checking the obvious...

	if( computed_bit_rate > (mSampleRateHz / 4) )
		return false; //the baud rate is too fast.
	if( computed_bit_rate == 0 )
	{
		//bad result, this is not good data, don't bother to re-run.
		return false;
	}

	U32 specified_bit_rate = mSettings->mBitRate;

	double error = double( AnalyzerHelpers::Diff32( computed_bit_rate, specified_bit_rate ) ) / double( specified_bit_rate );

	if( error > 0.1 )
	{
		mSettings->mBitRate = computed_bit_rate;
		mSettings->UpdateInterfacesFromSettings();
		return true;
	}else
	{
		return false;
	}
}

U32 ModbusAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 ModbusAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mBitRate * 4;
}

const char* ModbusAnalyzer::GetAnalyzerName() const
{
	return "Modbus";
}

const char* GetAnalyzerName()
{
	return "Modbus";
}

Analyzer* CreateAnalyzer()
{
	return new ModbusAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}

U64 ModbusAnalyzer::GetNextByteModbus(U32 num_bits, U64 bit_mask, U64 &frame_starting_sample, U64 &frame_ending_sample)
{	
	if(mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUMaster || mSettings->mModbusMode == ModbusAnalyzerEnums::ModbusRTUSlave)
	{
		mModbus->AdvanceToNextEdge();

			//we're now at the beginning of the start bit.  We can start collecting the data.
			frame_starting_sample = mModbus->GetSampleNumber();

			U64 data = 0;
			bool parity_error = false;
			bool framing_error = false;
			bool mp_is_address = false;
			
			DataBuilder data_builder;
			data_builder.Reset( &data, mSettings->mShiftOrder, num_bits );
			U64 marker_location = frame_starting_sample;

			for( U32 i=0; i<num_bits; i++ )
			{
				mModbus->Advance( mSampleOffsets[i] );
				data_builder.AddBit( mModbus->GetBitState() );

				marker_location += mSampleOffsets[i];
				mResults->AddMarker( marker_location, AnalyzerResults::Dot, mSettings->mInputChannel );
			}

			if( mSettings->mInverted == true )
				data = (~data) & bit_mask;
	
			parity_error = false;

			if( mSettings->mParity != AnalyzerEnums::None )
			{
				mModbus->Advance( mParityBitOffset );
				bool is_even = AnalyzerHelpers::IsEven( AnalyzerHelpers::GetOnesCount( data ) );

				if( mSettings->mParity == AnalyzerEnums::Even )
				{
					if( is_even == true )
					{
						if( mModbus->GetBitState() != mBitLow ) //we expect a low bit, to keep the parity even.
							parity_error = true;
					}else
					{
						if( mModbus->GetBitState() != mBitHigh ) //we expect a high bit, to force parity even.
							parity_error = true;
					}
				}else  //if( mSettings->mParity == AnalyzerEnums::Odd )
				{
					if( is_even == false )
					{
						if( mModbus->GetBitState() != mBitLow ) //we expect a low bit, to keep the parity odd.
							parity_error = true;
					}else
					{
						if( mModbus->GetBitState() != mBitHigh ) //we expect a high bit, to force parity odd.
							parity_error = true;
					}
				}

				marker_location += mParityBitOffset;
				mResults->AddMarker( marker_location, AnalyzerResults::Square, mSettings->mInputChannel );
			}

			mModbus->Advance( mStartOfStopBitOffset );
			
			frame_ending_sample = mModbus->GetSampleNumber();

		return data;
	}
	else
	{
		U8 value =0 ;
			//first get MSByte of ASCII data
			mModbus->AdvanceToNextEdge();

			//we're now at the beginning of the start bit.  We can start collecting the data.
			frame_starting_sample = mModbus->GetSampleNumber();

			U64 data = 0;
			bool parity_error = false;
			bool framing_error = false;
			bool mp_is_address = false;
			
			DataBuilder data_builder;
			data_builder.Reset( &data, mSettings->mShiftOrder, num_bits );
			U64 marker_location = frame_starting_sample;

			for( U32 i=0; i<num_bits; i++ )
			{
				mModbus->Advance( mSampleOffsets[i] );
				data_builder.AddBit( mModbus->GetBitState() );

				marker_location += mSampleOffsets[i];
				mResults->AddMarker( marker_location, AnalyzerResults::Dot, mSettings->mInputChannel );
			}

			if( mSettings->mInverted == true )
				data = (~data) & bit_mask;
	
			parity_error = false;

			if( mSettings->mParity != AnalyzerEnums::None )
			{
				mModbus->Advance( mParityBitOffset );
				bool is_even = AnalyzerHelpers::IsEven( AnalyzerHelpers::GetOnesCount( data ) );

				if( mSettings->mParity == AnalyzerEnums::Even )
				{
					if( is_even == true )
					{
						if( mModbus->GetBitState() != mBitLow ) //we expect a low bit, to keep the parity even.
							parity_error = true;
					}else
					{
						if( mModbus->GetBitState() != mBitHigh ) //we expect a high bit, to force parity even.
							parity_error = true;
					}
				}else  //if( mSettings->mParity == AnalyzerEnums::Odd )
				{
					if( is_even == false )
					{
						if( mModbus->GetBitState() != mBitLow ) //we expect a low bit, to keep the parity odd.
							parity_error = true;
					}else
					{
						if( mModbus->GetBitState() != mBitHigh ) //we expect a high bit, to force parity odd.
							parity_error = true;
					}
				}

				marker_location += mParityBitOffset;
				mResults->AddMarker( marker_location, AnalyzerResults::Square, mSettings->mInputChannel );
			}

			mModbus->Advance( mStartOfStopBitOffset );
			if(data==':' || data=='\n' || data=='\r')
				return data;
			else
			{
				value = ASCII2INT(data) << 4;

				//Gets the LSByte of ASCII data
			
				mModbus->AdvanceToNextEdge();

				//we're now at the beginning of the start bit.  We can start collecting the data.

				data = 0;
				parity_error = false;
				framing_error = false;
				mp_is_address = false;
				
				data_builder.Reset( &data, mSettings->mShiftOrder, num_bits );
				marker_location = mModbus->GetSampleNumber();

				for( U32 i=0; i<num_bits; i++ )
				{
					mModbus->Advance( mSampleOffsets[i] );
					data_builder.AddBit( mModbus->GetBitState() );

					marker_location += mSampleOffsets[i];
					mResults->AddMarker( marker_location, AnalyzerResults::Dot, mSettings->mInputChannel );
				}

				if( mSettings->mInverted == true )
					data = (~data) & bit_mask;
		
				parity_error = false;

				if( mSettings->mParity != AnalyzerEnums::None )
				{
					mModbus->Advance( mParityBitOffset );
					bool is_even = AnalyzerHelpers::IsEven( AnalyzerHelpers::GetOnesCount( data ) );

					if( mSettings->mParity == AnalyzerEnums::Even )
					{
						if( is_even == true )
						{
							if( mModbus->GetBitState() != mBitLow ) //we expect a low bit, to keep the parity even.
								parity_error = true;
						}else
						{
							if( mModbus->GetBitState() != mBitHigh ) //we expect a high bit, to force parity even.
								parity_error = true;
						}
					}else  //if( mSettings->mParity == AnalyzerEnums::Odd )
					{
						if( is_even == false )
						{
							if( mModbus->GetBitState() != mBitLow ) //we expect a low bit, to keep the parity odd.
								parity_error = true;
						}else
						{
							if( mModbus->GetBitState() != mBitHigh ) //we expect a high bit, to force parity odd.
								parity_error = true;
						}
					}

					marker_location += mParityBitOffset;
					mResults->AddMarker( marker_location, AnalyzerResults::Square, mSettings->mInputChannel );
				}

				mModbus->Advance( mStartOfStopBitOffset );
				
				frame_ending_sample = mModbus->GetSampleNumber();
			
				value = value + ASCII2INT(data);
			return value;
		}
	}
}

int ModbusAnalyzer::ASCII2INT(char value)
{
	switch(value)
	{
	case '0':
		return 0;
		break;
	case '1':
		return 1;
		break;
	case '2':
		return 2;
		break;
	case '3':
		return 3;
		break;
	case '4':
		return 4;
		break;
	case '5':
		return 5;
		break;
	case '6':
		return 6;
		break;
	case '7':
		return 7;
		break;
	case '8':
		return 8;
		break;
	case '9':
		return 9;
		break;
	case 'A':
		return 0xA;
		break;
	case 'B':
		return 0xB;
		break;
	case 'C':
		return 0xC;
		break;
	case 'D':
		return 0xD;
		break;
	case 'E':
		return 0xE;
		break;
	case 'F':
		return 0xF;
		break;
	default:
		return 0;
	}
}

U16 ModbusAnalyzer::update_CRC( U16 crc, U8 c )
{

    U16 tmp, short_c;

    short_c = 0x00ff & (U16) c;

    //if ( ! crc_tab16_init ) init_crc16_tab();

    tmp =  crc       ^ short_c;
    crc = (crc >> 8) ^ crc_tab16[ tmp & 0xff ];

    return crc;

} 

void ModbusAnalyzer::init_crc16_tab( void ) 
{

    int i, j;
    U16 crc, c;

    for (i=0; i<256; i++) {

        crc = 0;
        c   = (U16) i;

        for (j=0; j<8; j++) {

            if ( (crc ^ c) & 0x0001 ) crc = ( crc >> 1 ) ^ 0xA001;
            else                      crc =   crc >> 1;

            c = c >> 1;
        }

        crc_tab16[i] = crc;
    }

} 
