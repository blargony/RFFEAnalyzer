#include "MDIOAnalyzer.h"
#include "MDIOAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

MDIOAnalyzer::MDIOAnalyzer()
:	Analyzer2(),
mSettings( new MDIOAnalyzerSettings() ),
mSimulationInitialized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

MDIOAnalyzer::~MDIOAnalyzer()
{
	KillThread();
}

void MDIOAnalyzer::WorkerThread()
{	
	mSampleRateHz = GetSampleRate();
	// mPacketInTransaction = 0;
	// mTransactionID = 0;

	// mMDio and mMdc have the actual data from the channel
	mMdio = GetAnalyzerChannelData( mSettings->mMdioChannel );
	mMdc = GetAnalyzerChannelData( mSettings->mMdcChannel );

	for( ; ; )
	{
		// normally pulled up
		AdvanceToHighMDIO();

		// go to the beggining of a start frame (MDIO HIGH->LOW transition)
		AdvanceToStartFrame();

		// Put a marker in the start position
		mResults->AddMarker( mMdio->GetSampleNumber(), AnalyzerResults::Start, mSettings->mMdioChannel );

		// process the packet
		ProcessStartFrame();
		ProcessOpcodeFrame();
		ProcessPhyAddrFrame();
		ProcessRegAddrDevTypeFrame();
		ProcessTAFrame();
		ProcessAddrDataFrame();

		// Put a marker in the end of the packet
		mResults->AddMarker( mMdio->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mMdioChannel );

		AddArrowMarkers();

		// commit the generated frames to a packet
		U64 packet_id = mResults->CommitPacketAndStartNewPacket();

		// NOTE: For future support of transactions in MDIOAnalyzer class
		// add the current packet to the current transaction
		// mResults->AddPacketToTransaction( mTransactionID, packet_id );

		// finally commit the results to the MDIOAnalyzerResults class
		mResults->CommitResults();
		
		// NOTE: For future support of transactions in MDIOAnalyzer class
		// Check if it is the end of a C22 or C45 transaction (C45 transaction consists of two frames)
		/*
		if( ( mCurrentPacket.clause == MDIO_C22_PACKET ) ||
			( mCurrentPacket.clause == MDIO_C45_PACKET && mPacketInTransaction == 2 ) ) 
		{
			mTransactionID++;
			mPacketInTransaction = 0;
		}
		*/
	}
}

void MDIOAnalyzer::AddArrowMarkers() 
{
	// add arrows in clock posedges
	for( U32 i=0; i < mMdcPosedgeArrows.size(); i++ ) 
	{
		mResults->AddMarker( mMdcPosedgeArrows[i], AnalyzerResults::UpArrow, mSettings->mMdcChannel );
	}
	mMdcPosedgeArrows.clear();

	// add arrows in clock negedges
	for( U32 i=0; i < mMdcNegedgeArrows.size(); i++ ) 
	{
		mResults->AddMarker( mMdcNegedgeArrows[i], AnalyzerResults::DownArrow, mSettings->mMdcChannel );
	}
	mMdcNegedgeArrows.clear();
}

void MDIOAnalyzer::AdvanceToStartFrame() 
{
	if( mMdio->GetBitState() == BIT_LOW ) 
	{
		AnalyzerHelpers::Assert( "AdvanceToStartFrame() must be called with MDIO line with HIGH state" );
	}

	mMdio->AdvanceToNextEdge(); // high->low transition
	mMdc->AdvanceToAbsPosition( mMdio->GetSampleNumber() );
	if( mMdc->GetBitState( ) == BIT_HIGH )
		mMdc->AdvanceToNextEdge( );
}

void MDIOAnalyzer::ProcessStartFrame() 
{
	// starting sample of the start frame
	U64 starting_sample = mMdio->GetSampleNumber();

	// get the value of the two bits of the start frame
	BitState bit0, bit1;
	GetBit( bit0, mMdcPosedgeArrows ); // sample first bit 
	GetBit( bit1, mMdcPosedgeArrows ); // sample second bit

	// create and set the start frame
	Frame frame;
	frame.mType = ( bit1 == BIT_HIGH ) ? MDIO_C22_START : MDIO_C45_START;
	frame.mFlags = 0;
	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive = mMdio->GetSampleNumber();

	mResults->AddFrame( frame );
	ReportProgress( frame.mEndingSampleInclusive );

	mCurrentPacket.clause = ( frame.mType == MDIO_C22_START ) ? MDIO_C22_PACKET : MDIO_C45_PACKET;
	
	// NOTE: For future support of transactions in MDIOAnalyzer class
	/*
	if( mCurrentPacket.clause == MDIO_C45_PACKET ) 
	{
		mPacketInTransaction++;
	}
	*/
}

void MDIOAnalyzer::ProcessOpcodeFrame() 
{
	// starting sample of the start frame
	U64 starting_sample = mMdio->GetSampleNumber()+1;

	// get the value of the two bits of the start frame
	BitState bit0, bit1;
	GetBit( bit0, mMdcPosedgeArrows ); // sample first bit 
	GetBit( bit1, mMdcPosedgeArrows ); // sample second bit

	// reconstruct the opcode 
	DataBuilder opcode;
	U64 value=0;
	opcode.Reset( &value, AnalyzerEnums::MsbFirst, 2 );
	opcode.AddBit(bit0);
	opcode.AddBit(bit1);

	// create and set the opcode frame
	Frame frame;

	if( mCurrentPacket.clause == MDIO_C22_PACKET ) 
	{
		switch( value ) 
		{
			case C45_ADDRESS: 					
				frame.mType = MDIO_UNKNOWN; // Clause 22 opcode cannot be C45_ADDRESS (00)
				mCurrentPacket.operation = MDIO_PACKET_WRITE;
				break;	
			case C22_WRITE: 		
				frame.mType = MDIO_OP_W; 
				mCurrentPacket.operation = MDIO_PACKET_WRITE;
				break;
			case C22_READ:	
				frame.mType = MDIO_OP_R; 
				mCurrentPacket.operation = MDIO_PACKET_READ;
				break;
			case C45_READ:						
				frame.mType = MDIO_UNKNOWN;  // Clause 22 opcode cannot be C45_READ (11)
				mCurrentPacket.operation = MDIO_PACKET_READ;
				break;	
			default: 							
				frame.mType = MDIO_UNKNOWN;
		}
	}
	else // mCurrentPacket.clause == MDIO_C45_PACKET
	{
		mCurrentPacket.c45Type = MDIO_C45_PACKET_DATA;
		mCurrentPacket.operation = MDIO_PACKET_WRITE;
		
		switch( value ) 
		{
			case C45_ADDRESS: 					
				frame.mType = MDIO_C45_OP_ADDR;
				mCurrentPacket.c45Type = MDIO_C45_PACKET_ADDR;
				mCurrentPacket.operation = MDIO_PACKET_WRITE;
				break;	
			case C45_WRITE: 		
				frame.mType = MDIO_OP_W;
				mCurrentPacket.operation = MDIO_PACKET_WRITE;
				break;
			case C45_READ_AND_ADDR:	
				frame.mType = MDIO_C45_OP_READ_INC_ADDR; 
				mCurrentPacket.operation = MDIO_PACKET_READ;
				break;
			case C45_READ:						
				frame.mType = MDIO_OP_R; 
				mCurrentPacket.operation = MDIO_PACKET_READ;
				break;	
			default: 							
				frame.mType = MDIO_UNKNOWN;
		}
	}
	
	if( frame.mType == MDIO_UNKNOWN ) 
	{
		frame.mFlags = DISPLAY_AS_ERROR_FLAG;
	}
	else 
	{
		frame.mFlags = 0;
	}

	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive = mMdio->GetSampleNumber();

	mResults->AddFrame( frame );
	ReportProgress( frame.mEndingSampleInclusive );
}

void MDIOAnalyzer::ProcessPhyAddrFrame() 
{
	// starting sample of the start frame
	U64 starting_sample = mMdio->GetSampleNumber()+1;

	// get the value of the the 5 bits of the phyaddr frame
	DataBuilder opcode;
	U64 value;
	opcode.Reset( &value, AnalyzerEnums::MsbFirst, 5 );
	for(U32 i=0; i < 5; ++i) 
	{
		BitState bit;
		GetBit( bit, mMdcPosedgeArrows );
		opcode.AddBit(bit);
	}

	// create and set the phyaddr frame
	Frame frame;
	frame.mType = MDIO_PHYADDR;	
	frame.mData1 = value;
	frame.mFlags = 0;
	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive = mMdio->GetSampleNumber();

	mResults->AddFrame( frame );
	ReportProgress( frame.mEndingSampleInclusive );
}

void MDIOAnalyzer::ProcessRegAddrDevTypeFrame() 
{
	// starting sample of the start frame
	U64 starting_sample = mMdio->GetSampleNumber()+1;

	// get the value of the 5 bits of register address or devtype frame
	DataBuilder opcode;
	U64 value;
	opcode.Reset( &value, AnalyzerEnums::MsbFirst, 5 );
	for(U32 i=0; i < 5; ++i) 
	{
		BitState bit;
		GetBit( bit, mMdcPosedgeArrows );
		opcode.AddBit(bit);
	}

	// create and set the phyaddr frame
	Frame frame;
	if( mCurrentPacket.clause == MDIO_C22_PACKET ) 
	{
		frame.mType = MDIO_C22_REGADDR; 
	}
	else 
	{
		frame.mType = GetDevType(value);
	}
	frame.mData1 = value;
	frame.mFlags = 0;
	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive = mMdio->GetSampleNumber();

	mResults->AddFrame( frame );
	ReportProgress( frame.mEndingSampleInclusive );
}

void MDIOAnalyzer::ProcessTAFrame()
{
	if( mCurrentPacket.operation == MDIO_PACKET_READ ) 
	{
		ProcessTAFrameInReadPacket();
	}
	else // mCurrentPacket.operation == MDIO_PACKET_WRITE
	{
		ProcessTAFrameInWritePacket();
	}
}

void MDIOAnalyzer::ProcessTAFrameInWritePacket() 
{
	// starting sample of the TA frame
	U64 starting_sample = mMdio->GetSampleNumber()+1;

	// get the value of the two bits of the start frame
	BitState bit0, bit1;
	GetBit( bit0, mMdcPosedgeArrows); 
	GetBit( bit1, mMdcPosedgeArrows ); 

	Frame frame;
	frame.mType = MDIO_TA;
	// display as an error if the TA bits are not "10"
	frame.mFlags = ( bit0 == BIT_HIGH && bit1 == BIT_LOW ) ? 0 : DISPLAY_AS_ERROR_FLAG;
	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive = mMdio->GetSampleNumber();

	mResults->AddFrame( frame );
	ReportProgress( frame.mEndingSampleInclusive );
}

void MDIOAnalyzer::ProcessTAFrameInReadPacket() 
{
	// starting sample of the TA frame
	U64 starting_sample = mMdio->GetSampleNumber()+1;

	mMdc->AdvanceToNextEdge(); // posedge
	mMdc->AdvanceToNextEdge(); // negedge 

	mMdio->AdvanceToAbsPosition( mMdc->GetSampleNumber() );  // move to the mMdc negedge
	mMdcNegedgeArrows.push_back( mMdc->GetSampleNumber() );

	BitState bit_state = mMdio->GetBitState(); 				 // sample the mMdio channel

	U8 errorTA;
	if( bit_state == BIT_LOW ) // check if the slave didn't take the bus
	{
		errorTA = 0; // OK
	}
	else 
	{
		errorTA = DISPLAY_AS_ERROR_FLAG;
	}

	mMdc->AdvanceToNextEdge(); // posedge

	mMdio->AdvanceToAbsPosition( mMdc->GetSampleNumber() ); // advance mMdio 

	Frame frame;
	frame.mType = MDIO_TA;
	frame.mFlags = errorTA;
	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive = mMdio->GetSampleNumber();

	mResults->AddFrame( frame );
	ReportProgress( frame.mEndingSampleInclusive );
	// after this TA, the data frame will be sampled in the negedge of the MDC clock
}

void MDIOAnalyzer::ProcessAddrDataFrame() 
{
	U64 starting_sample = mMdio->GetSampleNumber()+1;

	DataBuilder opcode;
	U64 value;
	opcode.Reset( &value, AnalyzerEnums::MsbFirst, 16 );
	std::vector<U64> & arrows = ( mCurrentPacket.operation == MDIO_PACKET_READ ) 
	? mMdcNegedgeArrows : mMdcPosedgeArrows;
	for(U32 i=0; i < 16; ++i) 
	{
		BitState bit;
		GetBit( bit, arrows );
		opcode.AddBit(bit);
	}

	Frame frame;
	if( mCurrentPacket.clause == MDIO_C22_PACKET ) 
	{
		frame.mType = MDIO_C22_DATA;
	}
	else // mCurrentPacket.clause == MDIO_C45_PACKET
	{
		frame.mType =  ( mCurrentPacket.c45Type ==  MDIO_C45_PACKET_ADDR ) ? MDIO_C45_ADDR : MDIO_C45_DATA;
	}
	frame.mData1 = value;
	frame.mFlags = 0;
	frame.mStartingSampleInclusive = starting_sample;
	frame.mEndingSampleInclusive = mMdio->GetSampleNumber();
	
	mResults->AddFrame( frame );
	ReportProgress( frame.mEndingSampleInclusive );
}

void MDIOAnalyzer::AdvanceToHighMDIO() 
{
	if( mMdio->GetBitState() == BIT_LOW )
	{
		mMdio->AdvanceToNextEdge(); // go to high state
		mMdc->AdvanceToAbsPosition( mMdio->GetSampleNumber() );
	}
}

MDIOFrameType MDIOAnalyzer::GetDevType(const U64 & value)
{
	switch (value)
	{
		case DEV_RESERVED: 	return MDIO_C45_DEVTYPE_RESERVED;
		case DEV_PMD_PMA: 	return MDIO_C45_DEVTYPE_PMD_PMA;
		case DEV_WIS: 		return MDIO_C45_DEVTYPE_WIS;
		case DEV_PCS: 		return MDIO_C45_DEVTYPE_PCS;
		case DEV_PHY_XS: 	return MDIO_C45_DEVTYPE_PHY_XS;
		case DEV_DTE_XS: 	return MDIO_C45_DEVTYPE_DTE_XS;
		default: 			return MDIO_C45_DEVTYPE_OTHER;
	}
}

void MDIOAnalyzer::GetBit( BitState& bit_state, std::vector<U64> & arrows)
{
	mMdc->AdvanceToNextEdge(); // in write operation its a posedge, in read operation its a negedge
	U64 edge  = mMdc->GetSampleNumber();
	arrows.push_back( edge );

	mMdio->AdvanceToAbsPosition( edge );  // move mMdio to sample
	bit_state = mMdio->GetBitState(); // sample the mMdio channel

	mMdc->AdvanceToNextEdge(); // in write write operation its a nededge, in read operation its a posedge
	mMdio->AdvanceToAbsPosition( mMdc->GetSampleNumber() ); // advance mMdio 
}

bool MDIOAnalyzer::NeedsRerun()
{
    return false;
}

void MDIOAnalyzer::SetupResults()
{
    mResults.reset( new MDIOAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mMdioChannel );
}

U32 MDIOAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitialized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitialized = true;
	}
	
	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 MDIOAnalyzer::GetMinimumSampleRateHz()
{
	return 500000; // Minimum sampling rate: 500 Khz.
}

const char* MDIOAnalyzer::GetAnalyzerName() const
{
	return "MDIO";
}

const char* GetAnalyzerName()
{
	return "MDIO";
}

Analyzer* CreateAnalyzer()
{
	return new MDIOAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
