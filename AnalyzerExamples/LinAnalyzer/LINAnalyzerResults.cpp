#include "LINAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "LINAnalyzer.h"
#include "LINAnalyzerSettings.h"
#include <iostream>
#include <fstream>
#include <string>

LINAnalyzerResults::LINAnalyzerResults( LINAnalyzer* analyzer, LINAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

LINAnalyzerResults::~LINAnalyzerResults()
{
}


void LINAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

    char number_str[128];
    std::string fault_str;
	std::string str[3];

    if ( frame.mFlags & byteFramingError )      fault_str += "!FRAME";
    if ( frame.mFlags & headerBreakExpected )   fault_str += "!BREAK";
    if ( frame.mFlags & headerSyncExpected )    fault_str += "!SYNC" ;
    if ( frame.mFlags & checksumMismatch )      fault_str += "!CHK";
	if ( fault_str.length() )
    {
        fault_str += "!";
		AddResultString( fault_str.c_str() );
    }
	else
	{

		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		switch( (LINAnalyzerResults::tLINFrameState)frame.mType )
		{
			default:
			case LINAnalyzerResults::NoFrame:
				str[0] += "IBS";
				str[1] += "IB Space";
				str[2] += "Inter-Byte Space";
				//AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, str, 128 );
				break;
			case LINAnalyzerResults::headerBreak:			// expecting break.
				str[0] += "BRK";
				str[1] += "Break";
				str[2] += "Header Break";
				break;
			case LINAnalyzerResults::headerSync:			// expecting sync.
				str[0] += "SYN";
				str[1] += "Sync";
				str[2] += "Header Sync";
				break;
			case LINAnalyzerResults::headerPID:				// expecting PID.
				AnalyzerHelpers::GetNumberString( frame.mData1&0x3F, display_base, 8, number_str, 128 );
				str[0] += number_str;

				str[1] += "PID: ";
				str[1] += number_str;

				str[2] += "Protected ID: ";
				str[2] += number_str;
				break;
			// LIN Response
			case LINAnalyzerResults::responseDataZero:		// expecting first resppnse data byte.
			case LINAnalyzerResults::responseData:			// expecting response data.
				{
					char seq_str[128];
					AnalyzerHelpers::GetNumberString( frame.mData2-1, Decimal, 8, seq_str, 128 );
					str[0] += number_str;
					str[1] += "D"; str[1] += seq_str; str[1] += ": "; str[1] += number_str;
					str[2] += "Data "; str[2] += seq_str; str[2] += ": "; str[2] += number_str;
				}
				break;
			case LINAnalyzerResults::responseChecksum:		// expecting checksum.
				str[0] += number_str;
				str[1] += "CHK: "; str[1] += number_str;
				str[2] += "Checksum: "; str[2] += number_str;
				break;
		}

		AddResultString( str[0].c_str() );
		AddResultString( str[1].c_str() );
		AddResultString( str[2].c_str() );
	}
}

void LINAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
    char time_str[128];
    char number_str[128];
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "T.BREAK,BREAK,T.SYNC,SYNC,T.PID,PID,T.D,Dn..." << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
	    U32 j;
		Frame frame = GetFrame( i );

        if ( frame.mType == headerBreak )
        {
            for ( j=0; j < 12; j++ ) // max. LIN packet length
            {
                frame = GetFrame( i+j );
                AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );
                AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
                file_stream << time_str << "," << number_str << ",";

                if ( frame.mType == responseChecksum )
                {
                    break;
                }
            }

            AnalyzerHelpers::GetTimeString( frame.mEndingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );
            file_stream << time_str << std::endl;
            i+=j;
        }

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void LINAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
    ClearTabularText();

	Frame frame = GetFrame( frame_index );

    char number_str[128];
    std::string fault_str;
	std::string str;

    if ( frame.mFlags & byteFramingError )      fault_str += "!FRAME";
    if ( frame.mFlags & headerBreakExpected )   fault_str += "!BREAK";
    if ( frame.mFlags & headerSyncExpected )    fault_str += "!SYNC" ;
    if ( frame.mFlags & checksumMismatch )      fault_str += "!CHK";
	if ( fault_str.length() )
    {
        fault_str += "!";
		AddTabularText( fault_str.c_str() );
    }
	else
	{

		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		switch( (LINAnalyzerResults::tLINFrameState)frame.mType )
		{
			default:
			case LINAnalyzerResults::NoFrame:
				
				str += "Inter-Byte Space";
				//AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, str, 128 );
				break;

			case LINAnalyzerResults::headerBreak:			// expecting break.
				
				str += "Header Break";
				break;

			case LINAnalyzerResults::headerSync:			// expecting sync.
				
				str += "Header Sync";
				break;

			case LINAnalyzerResults::headerPID:				// expecting PID.

				AnalyzerHelpers::GetNumberString( frame.mData1&0x3F, display_base, 8, number_str, 128 );
				str += "Protected ID: ";
				str += number_str;
				break;

			// LIN Response
			case LINAnalyzerResults::responseDataZero:		// expecting first resppnse data byte.
			case LINAnalyzerResults::responseData:			// expecting response data.
				{
					char seq_str[128];
					AnalyzerHelpers::GetNumberString( frame.mData2-1, Decimal, 8, seq_str, 128 );
					str += "Data "; 
					str += seq_str; 
					str += ": "; 
					str += number_str;
				}
				break;
			case LINAnalyzerResults::responseChecksum:		// expecting checksum.
					str += "Checksum: "; 
					str += number_str;
				break;
		}

		AddTabularText( str.c_str() );
	}

}

void LINAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void LINAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
