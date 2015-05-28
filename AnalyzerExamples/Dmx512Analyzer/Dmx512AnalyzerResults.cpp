#include "Dmx512AnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "Dmx512Analyzer.h"
#include "Dmx512AnalyzerSettings.h"
#include <iostream>
#include <fstream>

Dmx512AnalyzerResults::Dmx512AnalyzerResults( Dmx512Analyzer* analyzer, Dmx512AnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

Dmx512AnalyzerResults::~Dmx512AnalyzerResults()
{
}

void Dmx512AnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& /*channel*/, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );
	
	char number_str[128];
	char value_str[128];
	//	AddResultString( number_str );
	switch (frame.mType) {
	case Dmx512Analyzer::BREAK:
		AddResultString( "BREAK" );
		AddResultString( "B" );
		break;
	case Dmx512Analyzer::MAB:
		if( frame.mFlags & DISPLAY_AS_ERROR_FLAG )
		{
			AddResultString( "!Warning: MAB is too short" );
			AddResultString( "!MAB is too short" );
			AddResultString( "!!!" );
			AddResultString( "!" );
		} else
		{
			AddResultString( "Mark After Break" );
			AddResultString( "MAB" );
			AddResultString( "MA" );
			AddResultString( "M" );
		}
		break;
	case Dmx512Analyzer::START_CODE:
		AnalyzerHelpers::GetNumberString( frame.mData2, display_base, 8, value_str, 128 );
		AddResultString( "START CODE: ", value_str );
		AddResultString( "START CODE" );
		AddResultString( "START" );
		AddResultString( "ST" );
		AddResultString( "S" );
		break;
	case Dmx512Analyzer::DATA:
		AnalyzerHelpers::GetNumberString( frame.mData1, Decimal, 9, number_str, 128 );
		AnalyzerHelpers::GetNumberString( frame.mData2, display_base, 8, value_str, 128 );
		AddResultString( "Slot ", number_str, ": ", value_str );
		AddResultString( number_str, ": ", value_str );
		AddResultString( number_str );
		break;
	case Dmx512Analyzer::MARK:
		AddResultString( "MARK Time after Slot" );
		AddResultString( "MARK Time" );
		AddResultString( "MARK" );
		AddResultString( "MAR" );
		AddResultString( "MA" );
		AddResultString( "M" );
		break;
	case Dmx512Analyzer::STOP:
		AddResultString( "stop bits" );
		AddResultString( "stop" );
		AddResultString( "s" );
		break;
	case Dmx512Analyzer::START:
		AddResultString( "start bit" );
		AddResultString( "start" );
		AddResultString("s");
		break;
	default:
		AnalyzerHelpers::Assert( "unexpected" );
	}
}

void Dmx512AnalyzerResults::GenerateExportFile( const char* file, DisplayBase /*display_base*/, U32 /*export_type_user_id*/ )
{
	std::ofstream file_stream( file, std::ios::out );
	
	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Values" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
	    Frame frame = GetFrame( i );
	    char time_str[128];
	    AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

	    switch( frame.mType )
		{
	    case Dmx512Analyzer::START_CODE:
			file_stream << time_str;
			break;
	    case Dmx512Analyzer::DATA:
			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData2, Decimal, 9, number_str, 4 );
			file_stream << "," << number_str;
			break;
	    case Dmx512Analyzer::BREAK:
			file_stream << std::endl;
			break;
	    case Dmx512Analyzer::MBB:
	    case Dmx512Analyzer::MAB:
	    case Dmx512Analyzer::MARK:
	    case Dmx512Analyzer::STOP:
	    case Dmx512Analyzer::START:
			break; /* minimum verbosity */
	    default:
		    file_stream << " Unknown Frame Type" << std::endl;
		} /* switch */
		
		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	} /* for */

	file_stream.close();
}

void Dmx512AnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
    ClearTabularText();

	Frame frame = GetFrame( frame_index );
	
	char number_str[128];
	char value_str[128];
	//	AddResultString( number_str );
	switch (frame.mType) {
	case Dmx512Analyzer::BREAK:
		AddTabularText( "BREAK" );
		break;
	case Dmx512Analyzer::MAB:
		if( frame.mFlags & DISPLAY_AS_ERROR_FLAG )
		{
			AddTabularText( "!Warning: MAB is too short" );
		} else
		{
			AddTabularText( "Mark After Break" );
		}
		break;
	case Dmx512Analyzer::START_CODE:
		AnalyzerHelpers::GetNumberString( frame.mData2, display_base, 8, value_str, 128 );
		AddTabularText( "START CODE: ", value_str );
		break;
	case Dmx512Analyzer::DATA:
		AnalyzerHelpers::GetNumberString( frame.mData1, Decimal, 9, number_str, 128 );
		AnalyzerHelpers::GetNumberString( frame.mData2, display_base, 8, value_str, 128 );
		AddTabularText( "Slot ", number_str, ": ", value_str );
		break;
	case Dmx512Analyzer::MARK:
		AddTabularText( "MARK Time after Slot" );
		break;
	case Dmx512Analyzer::STOP:
		AddTabularText( "stop bits" );
		break;
	case Dmx512Analyzer::START:
		AddTabularText( "start bit" );
		break;
	}
}

void Dmx512AnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void Dmx512AnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
