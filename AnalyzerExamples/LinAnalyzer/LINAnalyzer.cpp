#include "LINAnalyzer.h"
#include "LINAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

LINAnalyzer::LINAnalyzer()
: Analyzer()
, mSettings( new LINAnalyzerSettings() )
, mSimulationInitilized( false )
, mFrameState(LINAnalyzerResults::NoFrame)
{
	SetAnalyzerSettings( mSettings.get() );
}

LINAnalyzer::~LINAnalyzer()
{
	KillThread();
}

void LINAnalyzer::WorkerThread()
{
    bool showIBS=false;     // show inter-byte space?
	U8 nDataBytes=0;
	bool byteFramingError;
	Frame byteFrame;        // byte fame from start bit to stop bit
	Frame ibsFrame;         // inter-byte space startsing after SYNC

    ibsFrame.mData1 = 0;
    ibsFrame.mData2 = 0;
    ibsFrame.mFlags = 0;
    ibsFrame.mType = 0;

	mResults.reset( new LINAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );

	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );

	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();

	mResults->CancelPacketAndStartNewPacket();

	for( ; ; )
	{

        ibsFrame.mStartingSampleInclusive = mSerial->GetSampleNumber();
		if( ( mFrameState == LINAnalyzerResults::NoFrame ) || ( mFrameState == LINAnalyzerResults::headerBreak ) )
		{
			byteFrame.mData1 = GetBreakField( byteFrame.mStartingSampleInclusive, byteFrame.mEndingSampleInclusive, byteFramingError );
		}
		else
		{
			byteFrame.mData1 = ByteFrame( byteFrame.mStartingSampleInclusive, byteFrame.mEndingSampleInclusive, byteFramingError );
		}
		
		ibsFrame.mEndingSampleInclusive = byteFrame.mStartingSampleInclusive;
		byteFrame.mData2 = 0;
		byteFrame.mFlags = byteFramingError ? LINAnalyzerResults::byteFramingError : 0;
		byteFrame.mType = mFrameState;

        if ( showIBS )
        {
            mResults->AddFrame( ibsFrame );
        }

		switch( mFrameState )
		{
			case LINAnalyzerResults::NoFrame:
				mFrameState = LINAnalyzerResults::headerBreak;
			case LINAnalyzerResults::headerBreak:			// expecting break
                showIBS=true;
				if ( byteFrame.mData1 == 0x00 )
				{
					mFrameState = LINAnalyzerResults::headerSync;
					byteFrame.mType = LINAnalyzerResults::headerBreak;
				}
				else
				{
					byteFrame.mFlags |= LINAnalyzerResults::headerBreakExpected;
					mFrameState = LINAnalyzerResults::NoFrame;
				}
				break;
			case LINAnalyzerResults::headerSync:			// expecting sync.
				if ( byteFrame.mData1 == 0x55 )
				{
					mFrameState = LINAnalyzerResults::headerPID;
				}
				else
				{
					byteFrame.mFlags |= LINAnalyzerResults::headerSyncExpected;
					mFrameState = LINAnalyzerResults::NoFrame;
				}
				break;
			case LINAnalyzerResults::headerPID:				// expecting PID.
				mFrameState = LINAnalyzerResults::responseDataZero;
				if ( mSettings->mLINVersion >= 2 )
				{
					mChecksum.clear();
					mChecksum.add(byteFrame.mData1);
				}
				break;
			// LIN Response
			case LINAnalyzerResults::responseDataZero:		// expecting first resppnse data byte.
				if ( mSettings->mLINVersion < 2 )
				{
					mChecksum.clear();
				}
				mChecksum.add(byteFrame.mData1);
				nDataBytes = 1;
				mFrameState = LINAnalyzerResults::responseData;
				break;
			case LINAnalyzerResults::responseData:			// expecting response data.
				if ( nDataBytes >= 8 || mChecksum.result() == byteFrame.mData1 )
				{
					// FIXME - peek ahead for BREAK such that checksum match + BREAK detected at next char == end of packet.
					mFrameState = LINAnalyzerResults::responseChecksum;
					byteFrame.mType = LINAnalyzerResults::responseChecksum;
				}
				else
				{
					++nDataBytes;
					mChecksum.add( byteFrame.mData1 );
					break;
				}
			case LINAnalyzerResults::responseChecksum:		// expecting checksum.
				nDataBytes = 0;
				if ( mChecksum.result() != byteFrame.mData1 )
				{
					byteFrame.mFlags |= LINAnalyzerResults::checksumMismatch;
				}
				mFrameState = LINAnalyzerResults::NoFrame;
				showIBS=false;
				break;
		}

		byteFrame.mData2 = nDataBytes;

 		mResults->AddFrame( byteFrame );
		mResults->CommitPacketAndStartNewPacket();
		mResults->CommitResults();
		ReportProgress( byteFrame.mEndingSampleInclusive );
	}
}

bool LINAnalyzer::NeedsRerun()
{
	return false;
}

U32 LINAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 LINAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mBitRate * 4;
}

const char* LINAnalyzer::GetAnalyzerName() const
{
	return "LIN";
}

const char* GetAnalyzerName()
{
	return "LIN";
}

Analyzer* CreateAnalyzer()
{
	return new LINAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}

double LINAnalyzer::SamplesPerBit()
{
	return (double)GetSampleRate() / (double)mSettings->mBitRate;
}

double LINAnalyzer::HalfSamplesPerBit()
{
	return SamplesPerBit() * 0.5;
}

void LINAnalyzer::Advance(U16 nBits)
{
	mSerial->Advance( nBits * SamplesPerBit() );
}

void LINAnalyzer::AdvanceHalfBit()
{
	mSerial->Advance( HalfSamplesPerBit() );
}

U8 LINAnalyzer::GetBreakField( S64& startingSample, S64& endingSample, bool& framingError )
{
	// locate the start bit (falling edge expected)...
	S32 min_break_field_low_bits = 13; //as per the spec of LIN. at least 13 bits 
	S32 num_break_bits = 0;
	bool valid_fame = false;
	for( ;; )
	{
		mSerial->AdvanceToNextEdge();
		if( mSerial->GetBitState() == BIT_HIGH )
		{
			mSerial->AdvanceToNextEdge();
		}
		num_break_bits = ( ( mSerial->GetSampleOfNextEdge( ) - mSerial->GetSampleNumber( ) ) / SamplesPerBit( ) ) - 1;
		if( num_break_bits >= min_break_field_low_bits )
		{
			startingSample = mSerial->GetSampleNumber( );
			valid_fame = true;
			break;
		}
	}

	AdvanceHalfBit( );
	mResults->AddMarker( mSerial->GetSampleNumber( ), AnalyzerResults::Start, mSettings->mInputChannel );

	for( U32 i = 0; i<num_break_bits; i++ )
	{
		Advance( 1 );
		//let's put a dot exactly where we sample this bit:
		mResults->AddMarker( mSerial->GetSampleNumber( ),
							 mSerial->GetBitState( ) == BIT_HIGH ? AnalyzerResults::One : AnalyzerResults::Zero,
							 mSettings->mInputChannel );
	}

	// Validate the stop bit...
	Advance( 1 );
	if( mSerial->GetBitState( ) == BIT_HIGH )
	{
		mResults->AddMarker( mSerial->GetSampleNumber( ), AnalyzerResults::Stop, mSettings->mInputChannel );
		framingError = false;
	}
	else
	{
		mResults->AddMarker( mSerial->GetSampleNumber( ), AnalyzerResults::ErrorSquare, mSettings->mInputChannel );
		framingError = true;
	}

	endingSample = mSerial->GetSampleNumber( );

	return (valid_fame) ? 0 : 1;
}

U8 LINAnalyzer::ByteFrame( S64& startingSample, S64& endingSample, bool& framingError )
{
	U8 data = 0;
	U8 mask = 1;

	framingError=false;

	// locate the start bit (falling edge expected)...
	mSerial->AdvanceToNextEdge();
	if ( mSerial->GetBitState() == BIT_HIGH )
	{
		AdvanceHalfBit();
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::ErrorDot, mSettings->mInputChannel );
		mSerial->AdvanceToNextEdge();
		//framingError = true;
	}
	startingSample = mSerial->GetSampleNumber();
	AdvanceHalfBit();
	mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Start, mSettings->mInputChannel );

	// mark each data bit (LSB first)...
	for( U32 i=0; i<8; i++ )
	{
		Advance( 1 );

		if( mSerial->GetBitState() == BIT_HIGH )
			data |= mask;

		//let's put a dot exactly where we sample this bit:
		mResults->AddMarker( mSerial->GetSampleNumber(),
							mSerial->GetBitState() == BIT_HIGH ? AnalyzerResults::One : AnalyzerResults::Zero,
							mSettings->mInputChannel );

		mask = mask << 1;
	}

	// Validate the stop bit...
	Advance( 1 );
	if( mSerial->GetBitState() == BIT_HIGH )
	{
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Stop,mSettings->mInputChannel );
	}
	else
	{
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::ErrorSquare,mSettings->mInputChannel );
		framingError = true;
	}

	endingSample = mSerial->GetSampleNumber();

	return data;
}

