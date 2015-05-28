#include "OneWireAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "OneWireAnalyzer.h"
#include "OneWireAnalyzerSettings.h"
#include <iostream>
#include <sstream>

OneWireAnalyzerResults::OneWireAnalyzerResults( OneWireAnalyzer* analyzer, OneWireAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{

}

OneWireAnalyzerResults::~OneWireAnalyzerResults()
{

}

void OneWireAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& /*channel*/, DisplayBase display_base )  //unrefereced vars commented out to remove warnings.
{
	Frame frame = GetFrame( frame_index );
	ClearResultStrings();

	bool warning_flag = false;
	if( ( frame.mFlags & DISPLAY_AS_WARNING_FLAG ) != 0 )
		warning_flag = true;


	//RestartPulse, PresencePulse, ReadRomFrame, SkipRomFrame, SearchRomFrame, MatchRomFrame, OverdriveSkipRomFrame, OverdriveMatchRomFrame, CRC, FamilyCode, Rom, Byte, Bit
	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	
	switch( (OneWireFrameType)frame.mType )
	{
	case RestartPulse:
		{
			if( warning_flag == false )
			{
				AddResultString( "R" );
				AddResultString( "RESET" );
				AddResultString( "RESET condition" );
			}
			else
			{
				AddResultString( "R!" );
				AddResultString( "RESET - WARNING" );
				AddResultString( "RESET - warning, too short." );
				AddResultString( "RESET - warning, pulse shorter than 480us" );	
			}
		}
		break;
	case PresencePulse:
		{
			AddResultString( "P" );
			AddResultString( "PRESENCE" );
			AddResultString( "PRESENCE condition" );
		}
		break;
	case ReadRomFrame:
		{
			AddResultString( "READ" );
			AddResultString( "READ ROM COMMAND" );
			AddResultString( "READ ROM command: [", number_str, "]" );
		}
		break;
	case SkipRomFrame:
		{
			AddResultString( "SKIP" );
			AddResultString( "SKIP ROM COMMAND" );
			AddResultString( "SKIP ROM command: [", number_str, "]" );
		}
		break;
	case SearchRomFrame:
		{
			AddResultString( "SEARCH" );
			AddResultString( "SEARCH ROM COMMAND" );
			AddResultString( "SEARCH ROM command: [", number_str, "]" );
		}
		break;
	case MatchRomFrame:
		{
			AddResultString( "MATCH" );
			AddResultString( "MATCH ROM COMMAND" );
			AddResultString( "MATCH ROM command: [", number_str, "]" );
		}
		break;
	case OverdriveSkipRomFrame:
		{
			AddResultString( "OD SKIP" );
			AddResultString( "OVERDRIVE SKIP ROM" );
			AddResultString( "OVERDRIVE SKIP ROM command: [", number_str, "]" );
		}
		break;
	case OverdriveMatchRomFrame:
		{
			AddResultString( "OD MATCH" );
			AddResultString( "OVERDRIVE MATCH COMMAND" );
			AddResultString( "OVERDRIVE MATCH ROM command: [", number_str, "]" );
		}
		break;
	case CRC:
		{
			AddResultString( "CRC" );
			AddResultString( "CRC: [", number_str, "]" );
			AddResultString( "CRC section from ROM: [", number_str, "]" );
		}
		break;
	case FamilyCode:
		{
			AddResultString( "FAMILY" );
			AddResultString( "FAMILY: [", number_str, "]" );
			AddResultString( "FAMILY CODE section from ROM: [", number_str, "]" );
		}
		break;
	case Rom:
		{
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 48, number_str, 128 );
			AddResultString( "ROM" );
			AddResultString( "ROM: [", number_str, "]" );
			AddResultString( "ROM CODE section from ROM: [", number_str, "]" );
		}
		break;
	case Byte:
		{
			AddResultString( "D" );
			AddResultString( "DATA" );
			AddResultString( "DATA: [", number_str, "]" );
		}
		break;
	case Bit:
		{
			AddResultString( "Z" );
			AddResultString( "BIT" );
			AddResultString( "BIT - ERROR." );
		}
		break;
	case InvalidRomCommandFrame:
		{
			AddResultString( "COMMAND!" );
			AddResultString( "INVALID ROM COMMAND" );
			AddResultString( "Invalid ROM command: [", number_str, "]" );
		}
		break;
	}
}

void OneWireAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
{
	//export_type_user_id is only important if we have more than one export type.

	std::stringstream ss;
	void* f = AnalyzerHelpers::StartFile( file );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	ss << "PacketId, Time[s], Detail, [data]" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );

		bool warning_flag = false;
		if( ( frame.mFlags & DISPLAY_AS_WARNING_FLAG ) != 0 )
			warning_flag = true;
		
		char packet_id_str[128];
		U64 packet_id = GetPacketContainingFrameSequential( i );
		if( packet_id != INVALID_RESULT_INDEX )
			AnalyzerHelpers::GetNumberString( packet_id, Decimal, 0, packet_id_str, 128 );
		else
			packet_id_str[0] = 0;

		ss << packet_id_str << ",";

		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		ss << time_str << ",";
		//RestartPulse, PresencePulse, ReadRomFrame, SkipRomFrame, SearchRomFrame, MatchRomFrame, OverdriveSkipRomFrame, OverdriveMatchRomFrame, CRC, FamilyCode, Rom, Byte, Bit
		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128);
		switch( (OneWireFrameType)frame.mType )
		{
		case RestartPulse:
			{
				if( warning_flag == false ) //correct reset pulse.
					ss << "Reset Pulse";
				else //out of spec reset pulse.
					ss << "Reset Pulse ( out of spec )";
			}
			break;
		case PresencePulse:
			{
				ss << "Presence Pulse";
			}
			break;
		case ReadRomFrame:
			{
				ss << "Read Rom Command" << ", " << number_str;
			}
			break;
		case SkipRomFrame:
			{
				ss << "Skip Rom Command" << ", " << number_str;
			}
			break;
		case SearchRomFrame:
			{
				ss << "Search Rom Command" << ", " << number_str;
			}
			break;
		case MatchRomFrame:
			{
				ss << "Match Rom Command" << ", " << number_str;
			}
			break;
		case OverdriveSkipRomFrame:
			{
				ss << "Overdrive Skip Rom Command" << ", " << number_str;
			}
			break;
		case OverdriveMatchRomFrame:
			{
				ss << "Overdrive Match Rom Command" << ", " << number_str;
			}
			break;
		case CRC:
			{	
				ss << "ROM CRC" << ", " << number_str;
			}
			break;
		case FamilyCode:
			{
				ss << "ROM Family Code" << ", " << number_str;
			}
			break;
		case Rom:
			{
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 48, number_str, 128 );
				ss << "ROM Code" << ", " << number_str;

			}
			break;
		case Byte:
			{
				ss << "Data" << ", " << number_str;
			}
			break;
		case Bit:
			{
			}
			break;
		case InvalidRomCommandFrame:
			{
				ss << "Invalid Rom Command" << ", " << number_str;
			}
			break;
		}

		ss << std::endl;

		AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
		ss.str( std::string() );
							
		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			AnalyzerHelpers::EndFile( f );
			return;
		}
	}

	UpdateExportProgressAndCheckForCancel( num_frames, num_frames );
	AnalyzerHelpers::EndFile( f );
}

void OneWireAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );
    ClearTabularText();

	bool warning_flag = false;
	if( ( frame.mFlags & DISPLAY_AS_WARNING_FLAG ) != 0 )
		warning_flag = true;


	//RestartPulse, PresencePulse, ReadRomFrame, SkipRomFrame, SearchRomFrame, MatchRomFrame, OverdriveSkipRomFrame, OverdriveMatchRomFrame, CRC, FamilyCode, Rom, Byte, Bit
	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	
	switch( (OneWireFrameType)frame.mType )
	{
	case RestartPulse:
		{
			if( warning_flag == false )
			{
				AddTabularText( "RESET condition" );
			}
			else
			{
				AddTabularText( "RESET - warning, pulse shorter than 480us" );	
			}
		}
		break;
	case PresencePulse:
		{
			AddTabularText( "PRESENCE condition" );
		}
		break;
	case ReadRomFrame:
		{
			AddTabularText( "READ ROM command: [", number_str, "]" );
		}
		break;
	case SkipRomFrame:
		{
			AddTabularText( "SKIP ROM command: [", number_str, "]" );
		}
		break;
	case SearchRomFrame:
		{
			AddTabularText( "SEARCH ROM command: [", number_str, "]" );
		}
		break;
	case MatchRomFrame:
		{
			AddTabularText( "MATCH ROM command: [", number_str, "]" );
		}
		break;
	case OverdriveSkipRomFrame:
		{
			AddTabularText( "OVERDRIVE SKIP ROM command: [", number_str, "]" );
		}
		break;
	case OverdriveMatchRomFrame:
		{
			AddTabularText( "OVERDRIVE MATCH ROM command: [", number_str, "]" );
		}
		break;
	case CRC:
		{
			AddTabularText( "CRC section from ROM: [", number_str, "]" );
		}
		break;
	case FamilyCode:
		{
			AddTabularText( "FAMILY CODE section from ROM: [", number_str, "]" );
		}
		break;
	case Rom:
		{
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 48, number_str, 128 );
			AddTabularText( "ROM CODE section from ROM: [", number_str, "]" );
		}
		break;
	case Byte:
		{
			AddTabularText( "DATA: [", number_str, "]" );
		}
		break;
	case Bit:
		{
			AddTabularText( "BIT - ERROR." );
		}
		break;
	case InvalidRomCommandFrame:
		{
			AddTabularText( "Invalid ROM command: [", number_str, "]" );
		}
		break;
	}

}

void OneWireAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void OneWireAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
