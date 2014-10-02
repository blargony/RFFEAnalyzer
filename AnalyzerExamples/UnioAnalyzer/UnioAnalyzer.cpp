#include "UnioAnalyzer.h"
#include "UnioAnalyzerSettings.h" 
#include <AnalyzerChannelData.h>


UnioAnalyzer::UnioAnalyzer() 
:	Analyzer2(),
	mSettings( new UnioAnalyzerSettings() ),
	mSimulationInitilized( false ),
	mStandyPulseTimeS( 600E-6 ),
	mInputSpikeSuppressionTimeS( 75E-9 )
{
	SetAnalyzerSettings( mSettings.get() );
}

	double mGlitchTolerationTimeS;
	U32 mGlitchTolerationSamples;

UnioAnalyzer::~UnioAnalyzer()
{
	KillThread();
}

void UnioAnalyzer::SetupResults()
{
	mResults.reset( new UnioAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mScioChannel );
}

void UnioAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();

	mTimePerSample = 1.0 / double( mSampleRateHz );
	mStandyPulseSamples = U32( mStandyPulseTimeS / mTimePerSample ) + 1;
	mInputSpikeSuppressionSamples = U32( mInputSpikeSuppressionTimeS / mTimePerSample ) + 1;

	mScio = GetAnalyzerChannelData( mSettings->mScioChannel );

	MoveToFallingEdgeOfStandbyPulse();

	for( ; ; )
	{
		GetTransaction();

		mResults->CommitResults();
		ReportProgress( mScio->GetSampleNumber() );
		CheckIfThreadShouldExit();
	}
}

void UnioAnalyzer::AdvanceToNextEdgeIgnoreSpikes()
{
	for( ; ; )
	{
		mScio->AdvanceToNextEdge();
		if( mScio->WouldAdvancingCauseTransition( mInputSpikeSuppressionSamples ) == false )
			return;

		mResults->AddMarker( mScio->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mScioChannel );

		mScio->AdvanceToNextEdge();
	}
}

U32 UnioAnalyzer::GetNumEdgesBeforeSampleIgnoreSpikes( U64 sample )
{
	U32 edge_count = 0;

	while( mScio->WouldAdvancingToAbsPositionCauseTransition( sample ) == true )
	{
		mScio->AdvanceToNextEdge();
		
		if( mScio->WouldAdvancingCauseTransition( mInputSpikeSuppressionSamples ) == true )
		{
			//this is a glitch.
			mScio->AdvanceToNextEdge();

			if( mScio->GetSampleNumber() >= sample )
				break;
		}else
		{
			edge_count++;
		}
	}

	return edge_count;
}


void UnioAnalyzer::MoveToFallingEdgeOfStandbyPulse()
{
	if( mScio->GetBitState() == BIT_LOW )
		mScio->AdvanceToNextEdge();

	for( ; ; )
	{
		U64 beginning_sample = mScio->GetSampleNumber();

		AdvanceToNextEdgeIgnoreSpikes(); //negedge

		U64 samples_moved = mScio->GetSampleNumber() - beginning_sample;

		if( samples_moved >= mStandyPulseSamples )
			return;

		AdvanceToNextEdgeIgnoreSpikes(); //posedge
	}
}

void UnioAnalyzer::GetTransaction()
{
	mResults->CancelPacketAndStartNewPacket();  // just in case we exited last time due to an error

	//assumes that we always start right at the beginning of a header.
	UnioHeaderResult header_result = GetHeader();
	if( header_result == HeaderInvalid )
	{
		MoveToFallingEdgeOfStandbyPulse();
		return;
	}

	UnioByteResult address_result = GetAddress();
	if( address_result == ByteInvalid )
	{
		MoveToFallingEdgeOfStandbyPulse();
		return;
	}

	for( ; ; )
	{
		U8 data;
		UnioMasterAcknowledge masker_ack;
		UnioSlaveAcknowledge slave_ack;
		
		U64 start_of_byte = mBitStartingSample;


		UnioByteResult byte_result = GetByte( data, masker_ack, slave_ack );
		if( ( byte_result == ByteInvalid ) || ( slave_ack == NoSak ) )
		{
			MoveToFallingEdgeOfStandbyPulse();
			return;
		}

		Frame frame;
		frame.mStartingSampleInclusive = start_of_byte;
		frame.mEndingSampleInclusive = mBitStartingSample - 1;
		frame.mData1 = data;
		frame.mType = U8( DataFrame );
		frame.mFlags = 0;
		if( masker_ack == Mak )
			frame.mFlags |= MASTER_ACK;
		if( slave_ack == Sak )
			frame.mFlags |= SLAVE_ACK;
		mResults->AddFrame( frame );
		mResults->CommitResults();

		if( masker_ack != Mak )
		{
			//normal termination of sequence
			MoveToNextStartHeaderAfterNoMakSak();
			mResults->CommitPacketAndStartNewPacket();
			return;
		}
	}
}

void UnioAnalyzer::MoveToNextStartHeaderAfterNoMakSak()
{
	//the line should be high.
	if( mScio->GetBitState() == BIT_LOW )
		AnalyzerHelpers::Assert( "unexpected" );

	AdvanceToNextEdgeIgnoreSpikes();
}

UnioByteResult UnioAnalyzer::GetAddress()
{
	U8 address_1;
	U8 address_2;
	UnioMasterAcknowledge master_ack;
	UnioSlaveAcknowledge slave_ack;

	U64 start_of_address = mBitStartingSample;
	

	UnioByteResult byte_result = GetByte( address_1, master_ack, slave_ack );
	if( byte_result == ByteInvalid )
		return ByteInvalid;

	if( master_ack == NoMak )
	{
		Frame frame;
		frame.mStartingSampleInclusive = start_of_address;
		frame.mEndingSampleInclusive = mBitStartingSample - 1;
		frame.mType = U8( ErrorMakRequired );
		frame.mFlags = DISPLAY_AS_ERROR_FLAG;
		mResults->AddFrame( frame );
		mResults->CommitResults();

		return ByteInvalid;
	}

	Frame frame;
	frame.mType = U8( AddressFrame8 );

	U32 address = address_1;
	
	if( ( ( address_1 & 0xF0 ) ) == 0xF0 )
	{
		if( slave_ack == Sak )
		{
			Frame frame;
			frame.mStartingSampleInclusive = start_of_address;
			frame.mEndingSampleInclusive = mBitStartingSample - 1;
			frame.mType = U8( ErrorNoSakRequired );
			frame.mFlags = DISPLAY_AS_ERROR_FLAG;
			mResults->AddFrame( frame );
			mResults->CommitResults();
			return ByteInvalid;
		}	

		if( master_ack == NoMak )
		{
			Frame frame;
			frame.mStartingSampleInclusive = start_of_address;
			frame.mEndingSampleInclusive = mBitStartingSample - 1;
			frame.mType = U8( ErrorMakRequired );
			frame.mFlags = DISPLAY_AS_ERROR_FLAG;
			mResults->AddFrame( frame );
			mResults->CommitResults();
			return ByteInvalid;
		}	

		//12-bit address:
		byte_result = GetByte( address_2, master_ack, slave_ack );
		if( byte_result == ByteInvalid )
			return ByteInvalid;
		
		frame.mType = U8( AddressFrame12 );
		address &= 0x0F;
		address <<= 8;
		address |= address_2;
	}

	if(  master_ack == NoSak )
	{
		Frame frame;
		frame.mStartingSampleInclusive = start_of_address;
		frame.mEndingSampleInclusive = mBitStartingSample - 1;
		frame.mType = U8( ErrorMakRequired );
		frame.mFlags = DISPLAY_AS_ERROR_FLAG;
		mResults->AddFrame( frame );
		mResults->CommitResults();

		return ByteInvalid;
	}

	frame.mData1 = address;
	frame.mFlags = 0;
	if( master_ack == Mak )
		frame.mFlags |= MASTER_ACK;
	if( slave_ack == Sak )
		frame.mFlags |= SLAVE_ACK;

	frame.mStartingSampleInclusive = start_of_address;
	frame.mEndingSampleInclusive = mBitStartingSample - 1;
	mResults->AddFrame( frame );
	mResults->CommitResults();

	if( slave_ack == NoSak )
		return ByteInvalid;

	return ByteValid;
}


UnioByteResult UnioAnalyzer::GetByte( U8& data, UnioMasterAcknowledge& master_ack, UnioSlaveAcknowledge& slave_ack )
{
	data = 0;
	U8 mask = 1 << 7;
	for( U32 i=0; i<8; i++ )
	{
		UnioBitResult result = GetBit();

		if( result == Invalid )
		{
			Frame frame;
			frame.mType = U8( InvalidBit );
			frame.mFlags = DISPLAY_AS_ERROR_FLAG;
			frame.mStartingSampleInclusive = mBitStartingSample - mSamplesPerBit;
			frame.mEndingSampleInclusive = mBitStartingSample - 1;
			mResults->AddFrame( frame );
			mResults->CommitResults();

			return ByteInvalid;
		}

		if( result == High )
			data |= mask;
		
		mask >>= 1;
	}

	UnioBitResult result = GetMak();
	if( result == Invalid )
	{
		Frame frame;
		frame.mType = U8( InvalidBit );
		frame.mFlags = DISPLAY_AS_ERROR_FLAG;
		frame.mStartingSampleInclusive = mBitStartingSample - mSamplesPerBit;
		frame.mEndingSampleInclusive = mBitStartingSample - 1;
		mResults->AddFrame( frame );
		mResults->CommitResults();

		return ByteInvalid;
	}

	if( result == High )
		master_ack = Mak;
	else
		master_ack = NoMak;

	slave_ack = GetSak();

	return ByteValid;
}

UnioHeaderResult UnioAnalyzer::GetHeader()
{
	if( mScio->GetBitState() != BIT_LOW )
		AnalyzerHelpers::Assert("unexpected");

	U64 start_of_header = mScio->GetSampleNumber();
	mScio->AdvanceToNextEdge();

	U64 start_of_sync = mScio->GetSampleNumber();

	for( U32 i=0; i<9; i++ )
		mScio->AdvanceToNextEdge();

	U64 end_of_sync = mScio->GetSampleNumber();

	U32 num_samples = U32( end_of_sync - start_of_sync );
	double number_of_sync_samples = double( num_samples );

	mBitRate = 1.0 / ( ( number_of_sync_samples * .125 ) * mTimePerSample );
	mSamplesPerBit = U32( number_of_sync_samples * .125 );  // * 1/8
	mSamplesPerHalfBit = U32( number_of_sync_samples * .0625 );  // * 1/8   / 2
	mSamplesPerQuarterBit = U32( number_of_sync_samples * .03125 );  // 1/4 * 1/8
	mSamplesPerThreeQuartersBit = U32( number_of_sync_samples * .09375 ); // 3/4 * 1/8

	mBitStartingSample = end_of_sync;  //note we're assuming that the header has a MAK.  This is required, to end the sync period predictably. (it goes low at the beginning of the bit so it can go high later to make the MAK.)

	UnioBitResult mak_result = GetMak();
	if( mak_result == Invalid )
		return HeaderInvalid;

	if( mak_result == Low )
	{	
		Frame frame;
		frame.mType = U8( ErrorMakRequired );
		frame.mFlags = DISPLAY_AS_ERROR_FLAG;
		frame.mStartingSampleInclusive = start_of_header;
		frame.mEndingSampleInclusive = mBitStartingSample + mSamplesPerBit - 1;
		mResults->AddFrame( frame );
		mResults->CommitResults();

		return HeaderInvalid;
	}

	UnioSlaveAcknowledge sak_result = GetSak();
	if( sak_result == Sak )
	{
		Frame frame;
		frame.mType = U8( ErrorNoSakRequired );
		frame.mFlags = DISPLAY_AS_ERROR_FLAG;
		frame.mStartingSampleInclusive = start_of_header;
		frame.mEndingSampleInclusive = mBitStartingSample - 1;
		mResults->AddFrame( frame );
		mResults->CommitResults();

		return HeaderInvalid;
	}

	Frame frame;
	frame.mData1 = num_samples;
	frame.mType = U8( HeaderFrame );
	frame.mStartingSampleInclusive = start_of_header;
	frame.mEndingSampleInclusive = mBitStartingSample - 1;
	mResults->AddFrame( frame );
	mResults->CommitResults();

	return HeaderValid;
}



UnioBitResult UnioAnalyzer::GetBit( bool allow_mak_hold )
{
	U64 quarter_way = mBitStartingSample + mSamplesPerQuarterBit;

	U32 num_edges = this->GetNumEdgesBeforeSampleIgnoreSpikes( quarter_way );

	//edges in the this region are kindof don't cares.  
	
	U64 three_quarters_way = mBitStartingSample + mSamplesPerThreeQuartersBit;

	num_edges = this->GetNumEdgesBeforeSampleIgnoreSpikes( three_quarters_way );

	if( num_edges == 0 )
	{
		if( allow_mak_hold == false )
		{
			//bit has no change -- this is only valid for NoSAK
			mBitStartingSample += mSamplesPerBit;
			return Invalid;
		}else
		{
			//valid MAK hold condition.
			mScio->AdvanceToNextEdge();
			mBitStartingSample = mScio->GetSampleNumber() + mSamplesPerHalfBit;
			return High;  //Easy!
		}
	}

	if( num_edges > 1 )
	{
		Frame frame;
		frame.mType = U8( InvalidBit );
		frame.mFlags = DISPLAY_AS_ERROR_FLAG;
		frame.mStartingSampleInclusive = mBitStartingSample;
		frame.mEndingSampleInclusive = mBitStartingSample + mSamplesPerBit - 1;
		mResults->AddFrame( frame );
		mResults->CommitResults();

		return Invalid;
	}

	mBitStartingSample = mScio->GetSampleNumber() + mSamplesPerHalfBit; //align to this edge

	if( mScio->GetBitState() == BIT_HIGH )
	{
		mResults->AddMarker( mScio->GetSampleNumber(), AnalyzerResults::One, mSettings->mScioChannel );
		return High;
	}
	else
	{
		mResults->AddMarker( mScio->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mScioChannel );
		return Low;
	}
}

UnioBitResult UnioAnalyzer::GetMak()
{
	return GetBit( true );
}

UnioSlaveAcknowledge UnioAnalyzer::GetSak()
{
	UnioBitResult result = GetBit();
	if( result == Invalid )
		return NoSak;
	
	if( result == Low )
		return NoSak;

	return Sak;
}









bool UnioAnalyzer::NeedsRerun()
{
	return false;
}

U32 UnioAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 UnioAnalyzer::GetMinimumSampleRateHz()
{
	return 1000000;
}

const char* UnioAnalyzer::GetAnalyzerName() const
{
	return "UNI/O";
}

const char* GetAnalyzerName()
{
	return "UNI/O";
}

Analyzer* CreateAnalyzer()
{
	return new UnioAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
