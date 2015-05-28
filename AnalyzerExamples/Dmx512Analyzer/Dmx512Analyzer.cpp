#include "Dmx512Analyzer.h"
#include "Dmx512AnalyzerSettings.h"
#include <AnalyzerChannelData.h>

Dmx512Analyzer::Dmx512Analyzer()
:	Analyzer2(),
	mSettings( new Dmx512AnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

Dmx512Analyzer::~Dmx512Analyzer()
{
	KillThread();
}

void Dmx512Analyzer::WorkerThread()
{

	mSampleRateHz = GetSampleRate();
	
	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );
	mClockGenerator.Init( mSettings->mBitRate, mSampleRateHz );
	
	U64 start, cursor;
	
	// First we are going to look for a break
	
	do {
		if( mSerial->GetBitState() == BIT_HIGH )
			mSerial->AdvanceToNextEdge();
	    
		start = mSerial->GetSampleNumber();
		mSerial->AdvanceToNextEdge();  // LH
		cursor = mSerial->GetSampleNumber();
	} while ( cursor - start < mSampleRateHz * .000088 || cursor - start > mSampleRateHz );
	// not a true BREAK if low for <88us or >1s
	PassFrame( 0, BREAK, 0, start+1, cursor, 0 );
	
	for ( ; ; )
	{
		cursor = ReadMAB( cursor );
		cursor = ReadSlot( cursor, START_CODE, 0 );
		
		// What's next: a slot or the break?
		U64 next_edge = mSerial->GetSampleOfNextEdge(); // LH
		for ( int count=1; next_edge - cursor < mSampleRateHz * .000088; ++count ) {
			cursor = ReadSlot( cursor, DATA, count );
			next_edge = mSerial->GetSampleOfNextEdge(); // LH
		}
	  
		PassFrame( 0, BREAK, 0, cursor, next_edge, 0 );
		mSerial->AdvanceToAbsPosition( next_edge );
		cursor = next_edge;
	} // for (;;)
}

/* start: current GetSampleNumber() */
/* type: DATA or START_CODE */
/* count: if DATA, which slot in the packet we are reading */
/* returns: current GetSampleNumber() */
U64 Dmx512Analyzer::ReadSlot( U64 start, U8 type, int count )
{
	U64 cursor;
	// 1b low, 8b data, 2b high
	
	mSerial->Advance( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
	mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Start, mSettings->mInputChannel );
	mSerial->Advance( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
	U64 end = mSerial->GetSampleNumber();
	U8 flag = 0;
	PassFrame( count, START, flag, start+1, end, 0 );

	cursor = ReadByte( end, type, count );
	start = cursor;
	// 2 stop bits HIGH
	mSerial->Advance( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
	if( mSerial->GetBitState() == BIT_LOW )
		flag = DISPLAY_AS_ERROR_FLAG;
	mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mInputChannel );
	mSerial->Advance( mClockGenerator.AdvanceByHalfPeriod( 1 ) );
	if( mSerial->GetBitState() == BIT_LOW )
		flag = DISPLAY_AS_ERROR_FLAG;
	mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mInputChannel );
	mSerial->Advance( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
	end = mSerial->GetSampleNumber();
	PassFrame( 0, STOP, flag, start, end, 0 );

	// MBB OR Time Between Slots: may be 0
	if( mSerial->GetBitState() == BIT_LOW )
		return end; // Mark length is effectively 0

	start = end+1;
	mSerial->AdvanceToNextEdge(); // HL
	end = mSerial->GetSampleNumber();
	PassFrame( 0, MARK, flag, start, end, 0 );
	
	// either BREAK or START bit
	return end;
}

/* cursor: current GetSampleNumber() */
/* type: DATA or START_CODE */
/* count: which slot we are showing */
U64 Dmx512Analyzer::ReadByte( U64 cursor, U8 type, U64 count )
{
	U8 value = 0;
	for ( int i=0; i <= 7; ++i ) {
		mSerial->Advance( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
		AnalyzerResults::MarkerType marker;
		if( mSerial->GetBitState() == BIT_HIGH )
		{
			marker = AnalyzerResults::One;
			value |= 1<<i;
		} else
			marker = AnalyzerResults::Zero;

		mResults->AddMarker( mSerial->GetSampleNumber(), marker, mSettings->mInputChannel );
		mSerial->Advance( mClockGenerator.AdvanceByHalfPeriod( .5 ) );
  }
  U64 end = mSerial->GetSampleNumber();
  PassFrame( count, type, 0, cursor+1, end, value );
  return end;
}

/* start: end of last frame; must advance at least one */
/* returns mSerial->GetSampleNumber() */
U64 Dmx512Analyzer::ReadMAB( U64 start ) {
	mSerial->AdvanceToNextEdge(); // HL
	U64 end = mSerial->GetSampleNumber();
	U8 flag = 0;
	if(end - start < mSampleRateHz * mSettings->mMinMAB || end - start > mSampleRateHz )
	{
		mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mInputChannel );
		flag = DISPLAY_AS_ERROR_FLAG;
	}
	PassFrame( 0, MAB, flag, start+1, end, 0 );
	return end;
}

/* Populate and add a frame */
void Dmx512Analyzer::PassFrame( U64 data, U8 type, U8 flags, U64 start, U64 end, U8 data2 )
{
	Frame frame;
	frame.mData1 = data;
	frame.mData2 = data2;
	frame.mType = type;
	frame.mFlags = flags;
	frame.mStartingSampleInclusive = start;
	frame.mEndingSampleInclusive = end;
	mResults->AddFrame( frame );
	mResults->CommitResults();
	ReportProgress( frame.mEndingSampleInclusive );
}

bool Dmx512Analyzer::NeedsRerun()
{
    return false;
}

void Dmx512Analyzer::SetupResults()
{
    mResults.reset( new Dmx512AnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

U32 Dmx512Analyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 Dmx512Analyzer::GetMinimumSampleRateHz()
{
	return mSettings->mBitRate * 4;
}

const char* Dmx512Analyzer::GetAnalyzerName() const
{
	return "DMX-512";
}

const char* GetAnalyzerName()
{
	return "DMX-512";
}

Analyzer* CreateAnalyzer()
{
	return new Dmx512Analyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
