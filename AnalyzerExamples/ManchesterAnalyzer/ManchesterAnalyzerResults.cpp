#include "ManchesterAnalyzerResults.h"

#include <AnalyzerHelpers.h>
#include "ManchesterAnalyzer.h"
#include "ManchesterAnalyzerSettings.h"
#include <iostream>
#include <sstream>

ManchesterAnalyzerResults::ManchesterAnalyzerResults( ManchesterAnalyzer* analyzer, ManchesterAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{

}

ManchesterAnalyzerResults::~ManchesterAnalyzerResults()
{

}

void ManchesterAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& /*channel*/, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );
	ClearResultStrings();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer, number_str, 128 );
	AddResultString( number_str );
}

void ManchesterAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
{
	std::stringstream ss;
	void* f = AnalyzerHelpers::StartFile( file );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();
	U64 num_frames = GetNumFrames();

	ss << "Time [s],Data" << std::endl;

	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		//static void GetTimeString( U64 sample, U64 trigger_sample, U32 sample_rate_hz, char* result_string, U32 result_string_max_length );
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer, number_str, 128);

		ss << time_str << "," << number_str;
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

void ManchesterAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base )
{
    ClearTabularText();

	Frame frame = GetFrame( frame_index );
	
	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, mSettings->mBitsPerTransfer, number_str, 128 );
	AddTabularText( number_str );
}

void ManchesterAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )
{

}

void ManchesterAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )
{

}
