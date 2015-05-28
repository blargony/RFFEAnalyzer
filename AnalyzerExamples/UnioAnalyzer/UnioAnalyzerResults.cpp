#include "UnioAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "UnioAnalyzer.h"
#include "UnioAnalyzerSettings.h"
#include <iostream>
#include <sstream>
#include <stdio.h>

#pragma warning(disable: 4996) //warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.

UnioAnalyzerResults::UnioAnalyzerResults( UnioAnalyzer* analyzer, UnioAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

UnioAnalyzerResults::~UnioAnalyzerResults()
{
}

void UnioAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& /*channel*/, DisplayBase display_base )  //unrefereced vars commented out to remove warnings.
{
	//we only need to pay attention to 'channel' if we're making bubbles for more than one channel (as set by AddChannelBubblesWillAppearOn)
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	//enum UnioFrameType { HeaderFrame, AddressFrame8, AddressFrame12, DataFrame, InvalidBit, ErrorMakRequired, ErrorNoSakRequired };
	switch( UnioFrameType( frame.mType ) )
	{
	case HeaderFrame:
		{
			double bit_rate = double( mAnalyzer->GetSampleRate() ) / ( double( frame.mData1 ) * .125 );
			char frequency[256];
			sprintf( frequency, "%.1f kHz", bit_rate * .001 );

			AddResultString( "H" );
			AddResultString( "Header" );
			AddResultString( "Header; ", frequency );
			AddResultString( "Header; ", frequency );
			AddResultString( "Header; Bit-rate: ",  frequency );
		}
		break;
	case AddressFrame8:
	case AddressFrame12:
		{
			char number_str[128];
			if( UnioFrameType( frame.mType ) == AddressFrame8 )
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
			else
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 12, number_str, 128 );

			char master_ack[16];
			if( ( frame.mFlags & MASTER_ACK ) != 0 )
				sprintf( master_ack, "MAK" );
			else
				sprintf( master_ack, "NoMAK" );

			char slave_ack[16];
			if( ( frame.mFlags & SLAVE_ACK ) != 0 )
				sprintf( slave_ack, "SAK" );
			else
				sprintf( slave_ack, "NoSAK" );

			AddResultString( "A" );
			AddResultString( "A: ", number_str );
			AddResultString( "Address: ", number_str );
			AddResultString( "Address: ", number_str, "; ", master_ack, "; ", slave_ack );
		}
		break;
	case DataFrame:
		{
			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

			char master_ack[16];
			if( ( frame.mFlags & MASTER_ACK ) != 0 )
				sprintf( master_ack, "MAK" );
			else
				sprintf( master_ack, "NoMAK" );

			char slave_ack[16];
			if( ( frame.mFlags & SLAVE_ACK ) != 0 )
				sprintf( slave_ack, "SAK" );
			else
				sprintf( slave_ack, "NoSAK" );

			AddResultString( "D" );
			AddResultString( number_str );
			AddResultString( "Data: ", number_str );
			AddResultString( "Data: ", number_str, "; ", master_ack, "; ", slave_ack );
		}
		break;
	case ErrorMakRequired:
		AddResultString( "!" );
		AddResultString( "Error" );
		AddResultString( "Error: MAK is required" );
		break;
	case ErrorNoSakRequired:
		AddResultString( "!" );
		AddResultString( "Error" );
		AddResultString( "Error: NoSAK is required" );
		break;
	case InvalidBit:
		AddResultString( "!" );
		AddResultString( "Error" );
		AddResultString( "Error: Invalid UNIO bit" );
		break;
	default:
		AnalyzerHelpers::Assert( "unexpected" );
		break;
	}

}

void UnioAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
{
	//export_type_user_id is only important if we have more than one export type.

	std::stringstream ss;
	void* f = AnalyzerHelpers::StartFile( file );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	ss << "Time [s],Packet ID,Address,Data,MAK,SAK" << std::endl;

	U64 num_frames = GetNumFrames();
	char address_str[128] = "";
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );

		if( frame.mType == AddressFrame8 )
		{
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, address_str, 128 );
			if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
			{
				AnalyzerHelpers::EndFile( f );
				return;
			}
			continue;
		}

		if( frame.mType == AddressFrame12 )
		{
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, address_str, 128 );
			if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
			{
				AnalyzerHelpers::EndFile( f );
				return;
			}
			continue;
		}

		if( frame.mType == DataFrame )
		{
			char time_str[128];
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

			char data_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, data_str, 128 );

			ss << time_str << ",";

			U64 packet_id = GetPacketContainingFrameSequential( i );
			if( packet_id != INVALID_RESULT_INDEX )
				ss << packet_id;
	
			ss << "," << address_str << "," << data_str;

			if( ( frame.mFlags & MASTER_ACK ) != 0 )
				ss << ",MAK";
			else
				ss << ",NoMAK";

			if( ( frame.mFlags & SLAVE_ACK ) != 0 )
				ss << ",SAK";
			else
				ss << ",NoSAK";

			ss << std::endl;
		}

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

void UnioAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
    ClearTabularText();
	Frame frame = GetFrame( frame_index );

	//enum UnioFrameType { HeaderFrame, AddressFrame8, AddressFrame12, DataFrame, InvalidBit, ErrorMakRequired, ErrorNoSakRequired };
	switch( UnioFrameType( frame.mType ) )
	{
	case HeaderFrame:
		{
			double bit_rate = double( mAnalyzer->GetSampleRate() ) / ( double( frame.mData1 ) * .125 );
			char frequency[256];
			sprintf( frequency, "%.1f kHz", bit_rate * .001 );
			AddTabularText( "Header; Bit-rate: ",  frequency );
		}
		break;

	case AddressFrame8:
	case AddressFrame12:
		{
			char number_str[128];
			if( UnioFrameType( frame.mType ) == AddressFrame8 )
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
			else
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 12, number_str, 128 );

			char master_ack[16];
			if( ( frame.mFlags & MASTER_ACK ) != 0 )
				sprintf( master_ack, "MAK" );
			else
				sprintf( master_ack, "NoMAK" );

			char slave_ack[16];
			if( ( frame.mFlags & SLAVE_ACK ) != 0 )
				sprintf( slave_ack, "SAK" );
			else
				sprintf( slave_ack, "NoSAK" );

			AddTabularText( "Address: ", number_str, "; ", master_ack, "; ", slave_ack );
		}
		break;

	case DataFrame:
		{
			char number_str[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

			char master_ack[16];
			if( ( frame.mFlags & MASTER_ACK ) != 0 )
				sprintf( master_ack, "MAK" );
			else
				sprintf( master_ack, "NoMAK" );

			char slave_ack[16];
			if( ( frame.mFlags & SLAVE_ACK ) != 0 )
				sprintf( slave_ack, "SAK" );
			else
				sprintf( slave_ack, "NoSAK" );

			AddTabularText( "Data: ", number_str, "; ", master_ack, "; ", slave_ack );
		}
		break;

	case ErrorMakRequired:
		AddTabularText( "Error: MAK is required" );
		break;

	case ErrorNoSakRequired:
		AddTabularText( "Error: NoSAK is required" );
		break;

	case InvalidBit:
		AddTabularText( "Error: Invalid UNIO bit" );
		break;

	default:
		AnalyzerHelpers::Assert( "unexpected" );
		break;
	}
}

void UnioAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void UnioAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}
