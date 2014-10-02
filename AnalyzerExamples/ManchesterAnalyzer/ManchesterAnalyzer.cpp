#pragma warning(push, 0)
#include <sstream>
#include <ios>
#include <algorithm>
#pragma warning(pop)

#include "ManchesterAnalyzer.h"
#include "ManchesterAnalyzerSettings.h"  
#include <AnalyzerChannelData.h>


ManchesterAnalyzer::ManchesterAnalyzer()
:	mSettings( new ManchesterAnalyzerSettings() ),
	Analyzer2(),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

ManchesterAnalyzer::~ManchesterAnalyzer()
{
	KillThread();
}

void ManchesterAnalyzer::SetupResults()
{
	mResults.reset( new ManchesterAnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
}

void ManchesterAnalyzer::WorkerThread()
{
	mManchester = GetAnalyzerChannelData( mSettings->mInputChannel );

	mSampleRateHz = this->GetSampleRate();

	double half_peroid = 1.0 / double( mSettings->mBitRate * 2 );
	half_peroid *= 1000000.0;
	mT = U32( ( mSampleRateHz * half_peroid ) / 1000000.0 );
	switch( mSettings->mTolerance )
	{
	case TOL25:
		mTError = mT / 2;
		break;
	case TOL5:
		mTError = mT / 10;
		break;
	case TOL05:
		mTError = mT / 100;
		break;
	}
	if( mTError < 3 )
		mTError = 3;
	//mTError = mT / 2;
	mSynchronized = false;
	//mResults->AddMarker( mManchester->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
	mManchester->AdvanceToNextEdge();
	mBitsForNextByte.clear();
	mUnsyncedLocations.clear();
	mIgnoreBitCount = mSettings->mBitsToIgnore;


	for( ; ; )
	{
		switch( mSettings->mMode )
		{
		case MANCHESTER:
			{
				SynchronizeManchester();
				ProcessManchesterData();
			}
			break;
		case DIFFERENTIAL_MANCHESTER:
			{
				SynchronizeDifferential();
				ProcessDifferential();
			}
			break;
		case BI_PHASE_MARK:
			{
				SynchronizeBiPhase();
				ProcessBiPhaseData();
			}
			break;
		case BI_PHASE_SPACE:
			{
				SynchronizeBiPhase();
				ProcessBiPhaseData();
			}
			break;
		}
		//mManchester->AdvanceToNextEdge();
		//mResults->CommitResults();
		ReportProgress( mManchester->GetSampleNumber() );
		CheckIfThreadShouldExit();

	}
}

void ManchesterAnalyzer::ProcessBiPhaseData()
{

	if( mSynchronized == true )
	{
		U64 edge_location = mManchester->GetSampleNumber();

		mManchester->AdvanceToNextEdge();
		U64 next_edge_location = mManchester->GetSampleNumber();
		U64 edge_distance = next_edge_location - edge_location;
		if( ( edge_distance > ( (2*mT) - mTError ) ) && ( edge_distance < ( (2*mT) + mTError ) ) ) //long
		{
			if( mSettings->mMode == BI_PHASE_MARK ) //FM1
				SaveBit( edge_location, 0 );
			else if( mSettings->mMode == BI_PHASE_SPACE ) //FM0
				SaveBit( edge_location, 1 );
			return;
		}
		else if( ( edge_distance > ( mT - mTError ) ) && ( edge_distance < ( mT + mTError ) ) )	//short
		{
			if( mSettings->mMode == BI_PHASE_MARK ) //FM1
				SaveBit( edge_location, 1 );
			else if( mSettings->mMode == BI_PHASE_SPACE ) //FM0
				SaveBit( edge_location, 0 );			

			edge_location = mManchester->GetSampleNumber();
			mManchester->AdvanceToNextEdge();
			next_edge_location = mManchester->GetSampleNumber();
			edge_distance = next_edge_location - edge_location;
			if( ( edge_distance > ( mT - mTError ) ) && ( edge_distance < ( mT + mTError ) ) ) //short.
			{
				return;
			}
			else
			{
				Invalidate();
				return;
				//not synced anymore.
			}
		}
		else
		{
			//record first, then invalidate. (long)
			if( mSettings->mMode == BI_PHASE_MARK ) //FM1
				SaveBit( edge_location, 0 );
			else if( mSettings->mMode == BI_PHASE_SPACE ) //FM0
				SaveBit( edge_location, 1 );	
			Invalidate();
			return;
			//not synced anymore.
		}
	}
	return;

	
}

void ManchesterAnalyzer::SynchronizeBiPhase()
{
	while( mSynchronized == false )
	{
		CheckIfThreadShouldExit();
		U64 edge_location = mManchester->GetSampleNumber();
		mManchester->AdvanceToNextEdge();
		U64 next_edge_location = mManchester->GetSampleNumber();
		U64 edge_distance = next_edge_location - edge_location;
		if( ( edge_distance > ( mT - mTError ) ) && ( edge_distance < ( mT + mTError ) ) ) //short
		{
			mUnsyncedLocations.push_back( edge_location );
		}
		else if( ( edge_distance > ( (2*mT) - mTError ) ) && ( edge_distance < ( (2*mT) + mTError ) ) ) //long
		{
			mUnsyncedLocations.push_back( edge_location );
			mSynchronized = true;
			U32 bit_value = 0;
			if( mSettings->mMode == BI_PHASE_MARK ) //FM1
				bit_value = 1;
			else if( mSettings->mMode == BI_PHASE_SPACE ) //FM0
				bit_value = 0;

			std::vector<U64> locations_to_save;
			while( mUnsyncedLocations.size() > 0 )
			{
				locations_to_save.push_back( mUnsyncedLocations.back() );
				mUnsyncedLocations.pop_back();
				if( mUnsyncedLocations.size() > 0 )
					mUnsyncedLocations.pop_back();
			}
			std::sort( locations_to_save.begin(), locations_to_save.end() );
			U32 count = locations_to_save.size();
			for( U32 i = 0; i < count; ++i )
			{
				if( i == (count-1) )
					SaveBit( locations_to_save[i], bit_value ^ 0x1 );
				else
					SaveBit( locations_to_save[i], bit_value );
			}
			break;
		}
		else
		{
			//back to idle.
			Invalidate();
		}
	}
}

void ManchesterAnalyzer::ProcessManchesterData()
{
	if( mSynchronized == true )
	{
		U64 edge_location = mManchester->GetSampleNumber();
		BitState current_bit_state = mManchester->GetBitState();
		if( ( mSettings->mInverted == false ) && ( current_bit_state == BIT_HIGH ) )	//pos edge, zero
			SaveBit( edge_location, 0 );
		else if( ( mSettings->mInverted == false ) && ( current_bit_state == BIT_LOW ) )	//neg edge, one
			SaveBit( edge_location, 1 );
		else if( ( mSettings->mInverted == true ) && ( current_bit_state == BIT_HIGH ) )	//pos edge, inverted, one
			SaveBit( edge_location, 1 );
		else if( ( mSettings->mInverted == true ) && ( current_bit_state == BIT_LOW ) )		//neg edge, inverted, zero
			SaveBit( edge_location, 0 );

		mManchester->AdvanceToNextEdge();
		U64 next_edge_location = mManchester->GetSampleNumber();
		U64 edge_distance = next_edge_location - edge_location;
		if( ( edge_distance > ( (2*mT) - mTError ) ) && ( edge_distance < ( (2*mT) + mTError ) ) ) //long
		{
			return;
		}
		else if( ( edge_distance > ( mT - mTError ) ) && ( edge_distance < ( mT + mTError ) ) )	//short
		{
			edge_location = mManchester->GetSampleNumber();
			mManchester->AdvanceToNextEdge();
			next_edge_location = mManchester->GetSampleNumber();
			edge_distance = next_edge_location - edge_location;
			if( ( edge_distance > ( mT - mTError ) ) && ( edge_distance < ( mT + mTError ) ) ) //second short (good)
				return;
			else
			{
				Invalidate();
				return;
				//not synced anymore.
			}
		}
		else
		{
			Invalidate();
			return;
			//not synced anymore.
		}
	}
	return;
}

void ManchesterAnalyzer::SynchronizeManchester()
{
	//we should already be on an edge, the first one out of idle.
	while( mSynchronized == false )
	{
		CheckIfThreadShouldExit();
		U64 edge_location = mManchester->GetSampleNumber();
		mManchester->AdvanceToNextEdge();
		U64 next_edge_location = mManchester->GetSampleNumber();
		U64 edge_distance = next_edge_location - edge_location;
		if( ( edge_distance > ( mT - mTError ) ) && ( edge_distance < ( mT + mTError ) ) ) //short
		{
			mUnsyncedLocations.push_back( edge_location );
		}
		else if( ( edge_distance > ( (2*mT) - mTError ) ) && ( edge_distance < ( (2*mT) + mTError ) ) ) //long
		{
			mUnsyncedLocations.push_back( edge_location );
			mSynchronized = true;
			BitState current_bit_state = mManchester->GetBitState();
			bool inverted = mSettings->mInverted;
			U32 bit_value = 0;
			if( ( inverted == true ) && ( current_bit_state == BIT_LOW ) ) //we are on a neg edge with inverted true
				bit_value = 1;	//the old marker will be the opposite of the current value.
			else if( ( inverted == true ) && ( current_bit_state == BIT_HIGH ) )
				bit_value = 0;
			else if( ( inverted == false ) && ( current_bit_state == BIT_LOW ) ) //we are on a neg edge with inverted false
				bit_value = 0;
			else if( ( inverted == false ) && ( current_bit_state == BIT_HIGH ) )
				bit_value = 1;
			std::vector<U64> locations_to_save;
			while( mUnsyncedLocations.size() > 0 )
			{
				locations_to_save.push_back( mUnsyncedLocations.back() );
				mUnsyncedLocations.pop_back();
				if( mUnsyncedLocations.size() > 0 )
					mUnsyncedLocations.pop_back();
			}
			std::sort( locations_to_save.begin(), locations_to_save.end() );
			U32 count = locations_to_save.size();
			for( U32 i = 0; i < count; ++i )
				SaveBit( locations_to_save[i], bit_value );
			break;
		}
		else
		{
			//back to idle.
			Invalidate();
		}
	}
}

void ManchesterAnalyzer::ProcessDifferential()
{
	//we should be on a clock edge that has already been recorded.
	if( mSynchronized == true )
	{
		U64 edge_location = mManchester->GetSampleNumber();

		mManchester->AdvanceToNextEdge();
		U64 next_edge_location = mManchester->GetSampleNumber();
		U64 edge_distance = next_edge_location - edge_location;
		if( ( edge_distance > ( (2*mT) - mTError ) ) && ( edge_distance < ( (2*mT) + mTError ) ) ) //long
		{
			SaveBit( next_edge_location, 1 );
			return;
		}
		else if( ( edge_distance > ( mT - mTError ) ) && ( edge_distance < ( mT + mTError ) ) )	//short
		{

			edge_location = mManchester->GetSampleNumber();
			mManchester->AdvanceToNextEdge();
			next_edge_location = mManchester->GetSampleNumber();
			edge_distance = next_edge_location - edge_location;
			if( ( edge_distance > ( mT - mTError ) ) && ( edge_distance < ( mT + mTError ) ) ) //short.
			{
				SaveBit( next_edge_location, 0 );
				return;
			}
			else
			{
				Invalidate();
				return;
				//not synced anymore.
			}
		}
		else
		{
			Invalidate();
			return;
			//not synced anymore.
		}
	}
	return;
}

void ManchesterAnalyzer::SynchronizeDifferential()
{
	while( mSynchronized == false )
	{
		CheckIfThreadShouldExit();
		U64 edge_location = mManchester->GetSampleNumber();
		mManchester->AdvanceToNextEdge();
		U64 next_edge_location = mManchester->GetSampleNumber();
		U64 edge_distance = next_edge_location - edge_location;
		if( ( edge_distance > ( mT - mTError ) ) && ( edge_distance < ( mT + mTError ) ) ) //short
		{
			mUnsyncedLocations.push_back( edge_location );
		}
		else if( ( edge_distance > ( (2*mT) - mTError ) ) && ( edge_distance < ( (2*mT) + mTError ) ) ) //long
		{
			mUnsyncedLocations.push_back( edge_location );
			mSynchronized = true;

			bool last_bit_one = false;
			if( mUnsyncedLocations.size() % 2 == 1 )
				last_bit_one = true;
			std::vector<U64> locations_to_save;
			while( mUnsyncedLocations.size() > 0 )
			{
				locations_to_save.push_back( mUnsyncedLocations.back() );
				mUnsyncedLocations.pop_back();
				if( mUnsyncedLocations.size() > 0 )
					mUnsyncedLocations.pop_back();
			}
			std::sort( locations_to_save.begin(), locations_to_save.end() );
			U32 count = locations_to_save.size();
			for( U32 i = 0; i < count; ++i )
			{
				if( ( i == 0 ) && ( last_bit_one == true ) )
					SaveBit( locations_to_save[i], 1 );
				else
					SaveBit( locations_to_save[i], 0 );
			}

			SaveBit( next_edge_location, 1 );
			break;
		}
		else
		{
			//back to idle.
			Invalidate();
		}
	}
}

void ManchesterAnalyzer::SaveBit( U64 location, U32 value )
{
	if( mIgnoreBitCount == 0 )
		mBitsForNextByte.push_back( std::pair< U64, U64 >( value, location ) );
	else
		--mIgnoreBitCount;
	if( value == 1 )
		mResults->AddMarker( location, AnalyzerResults::One, mSettings->mInputChannel );
	else if( value == 0 )
		mResults->AddMarker( location, AnalyzerResults::Zero, mSettings->mInputChannel );

	U32 bit_count = mSettings->mBitsPerTransfer;
	if( mBitsForNextByte.size() == bit_count )
	{
		U64 byte = 0;
		if( mSettings->mShiftOrder == AnalyzerEnums::MsbFirst )
		{
			for( U32 i = 0; i < bit_count; ++i )
				byte |= ( mBitsForNextByte[i].first << ( bit_count - i - 1 ) );
		}
		else if( mSettings->mShiftOrder == AnalyzerEnums::LsbFirst )
		{
			for( U32 i = 0; i < bit_count; ++i )
				byte |= ( mBitsForNextByte[i].first << i );
		}
		Frame frame;
		frame.mStartingSampleInclusive = mBitsForNextByte[0].second - ( mT / 2 );
		frame.mEndingSampleInclusive = location + ( mT / 2 );
		frame.mData1 = byte;
		mResults->AddFrame( frame );
		mBitsForNextByte.clear();
		mResults->CommitResults();
		ReportProgress( mManchester->GetSampleNumber() );
	}
}

void ManchesterAnalyzer::Invalidate()
{
	mSynchronized = false;
	mBitsForNextByte.clear();
	mUnsyncedLocations.clear();
	mIgnoreBitCount = mSettings->mBitsToIgnore;
}

void ManchesterAnalyzer::ProcessSpace( U32 half_periods )
{

	//1T = can't be used to syncronize, usually. 

	//2T = can syncronize anything.
	if( half_periods == 1 )
	{
		
	}
	else if( half_periods == 2 )
	{
		if( mSynchronized == false )
		{
			//syncronize here. we are on a clock edge.
			mSynchronized = true;



			
		}
	}

	switch( mSettings->mMode )
	{
	case MANCHESTER:
		{
			/*
			if( mSettings->mInverted == false )
			{

			}
			else
			{

			}*/
		}
		break;
	case DIFFERENTIAL_MANCHESTER:
		{

		}
		break;
	case BI_PHASE_MARK:
		{
			ProcessBiPhaseData();
		}
		break;
	case BI_PHASE_SPACE:
		{
			ProcessBiPhaseData();
		}
		break;
	}
}

U32 ManchesterAnalyzer::GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( newest_sample_requested, sample_rate, simulation_channels );
}

U32 ManchesterAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mBitRate * 8;
}

bool ManchesterAnalyzer::NeedsRerun()
{
	return false;
}
const char gAnalyzerName[] = "Manchester";  //your analyzer must have a unique name

const char* ManchesterAnalyzer::GetAnalyzerName() const
{
	return gAnalyzerName;
}

const char* GetAnalyzerName()
{
	return gAnalyzerName;
}

Analyzer* CreateAnalyzer()
{
	return new ManchesterAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
