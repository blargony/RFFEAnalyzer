#include "SimpleParallelAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "SimpleParallelAnalyzer.h"
#include "SimpleParallelAnalyzerSettings.h"
#include <iostream>
#include <sstream>
#include <stdio.h>

SimpleParallelAnalyzerResults::SimpleParallelAnalyzerResults( SimpleParallelAnalyzer* analyzer, SimpleParallelAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

SimpleParallelAnalyzerResults::~SimpleParallelAnalyzerResults()
{
}

void SimpleParallelAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 16, number_str, 128 );
	AddResultString( number_str );
}

void SimpleParallelAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::stringstream ss;
	void* f = AnalyzerHelpers::StartFile( file );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	ss << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 16, number_str, 128 );

		ss << time_str << "," << number_str << std::endl;

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

void SimpleParallelAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
    ClearTabularText();
	Frame frame = GetFrame( frame_index );

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 16, number_str, 128 );
	AddTabularText( number_str );
}

void SimpleParallelAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void SimpleParallelAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
