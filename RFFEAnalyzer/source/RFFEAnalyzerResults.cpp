#include "RFFEAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "RFFEAnalyzer.h"
#include "RFFEAnalyzerSettings.h"
#include <iostream>
#include <sstream>

static const char *RffeTypeStringShort[] =
{
    "EW",
    "-",
    "ER",
    "ELW",
    "ELR",
    "W",
    "R",
    "W0",
};

static const char *RffeTypeStringMid[] =
{
    "ExtWr",
    "Rsv",
    "ExtRd",
    "ExtLngWr",
    "ExtLngRd",
    "Wr",
    "Rd",
    "Wr0",
};


RFFEAnalyzerResults::RFFEAnalyzerResults( RFFEAnalyzer* analyzer, RFFEAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

RFFEAnalyzerResults::~RFFEAnalyzerResults()
{
}

void RFFEAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
    channel = channel;

	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

    switch( frame.mType )
    {
    case RffeSSCField:
        {
            AddResultString( "SSC" );
        }
        break;

    case RffeSAField:
        {
            char number_str[8];
		    std::stringstream ss;

		    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 4, number_str, 8 );

            AddResultString( "SA" );

		    ss << "SA:" << number_str;
		    AddResultString( ss.str().c_str() );
        }
        break;

    case RffeTypeField:
        {
            AddResultString( RffeTypeStringShort[frame.mData1] );
            AddResultString( RffeTypeStringMid[frame.mData1] );
        }
        break;

    case RffeExByteCountField:
        {
            char number_str[8];
		    std::stringstream ss;

		    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 4, number_str, 8 );

            AddResultString( "BC" );

		    ss << "BC:" << number_str;
		    AddResultString( ss.str().c_str() );
        }
        break;

    case RffeExLongByteCountField:
        {
            char number_str[8];
		    std::stringstream ss;

		    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 3, number_str, 8 );

            AddResultString( "BC" );

		    ss << "BC:" << number_str;
		    AddResultString( ss.str().c_str() );
            ss.str("");
        }
        break;

    case RffeShortAddressField:
        {
            char number_str[8];
		    std::stringstream ss;

		    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 5, number_str, 8 );

            AddResultString( "A" );

		    ss << "A:" << number_str;
		    AddResultString( ss.str().c_str() );
        }
        break;

    case RffeAddressField:
        {
            char number_str[8];
		    std::stringstream ss;

		    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 8 );

            switch( frame.mData2 )
            {
            case RffeAddressHiField:
                AddResultString( "A" );

		        ss << "AH:" << number_str;
		        AddResultString( ss.str().c_str() );
                break;
            case RffeAddressLoField:
                AddResultString( "A" );

		        ss << "AL:" << number_str;
		        AddResultString( ss.str().c_str() );
                break;
            case RffeAddressNormalField:
            default:
                AddResultString( "A" );

		        ss << "A:" << number_str;
		        AddResultString( ss.str().c_str() );
            }
        }
        break;

    case RffeShortDataField:
        {
            char number_str[8];
		    std::stringstream ss;

		    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 7, number_str, 8 );

            AddResultString( "D" );

		    ss << "D:" << number_str;
		    AddResultString( ss.str().c_str() );
        }
        break;

    case RffeDataField:
        {
            char number_str[8];
		    std::stringstream ss;

		    AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 8 );

            AddResultString( "D" );

		    ss << "D:" << number_str;
		    AddResultString( ss.str().c_str() );
        }
        break;

    case RffeParityField:
        {
            char number_str[4];
		    std::stringstream ss;

		    AnalyzerHelpers::GetNumberString( frame.mData1, Decimal, 1, number_str, 4 );

            AddResultString( "P" );

		    ss << "P" << number_str;
		    AddResultString( ss.str().c_str() );
        }
        break;

    case RffeBusParkField:
        {
            AddResultString( "B" );
            AddResultString( "BP" );
        }
        break;

    case RffeErrorCaseField:
    default:
        {
            char number1_str[20];
            char number2_str[20];
		    std::stringstream ss;

		    AnalyzerHelpers::GetNumberString( frame.mData1, Hexadecimal, 32, number1_str, 10 );
		    AnalyzerHelpers::GetNumberString( frame.mData2, Hexadecimal, 32, number2_str, 10 );

            AddResultString( "E" );

		    ss << "E:" << number1_str << " - " << number2_str;
		    AddResultString( ss.str().c_str() );
        }
        break;
    }
}

void RFFEAnalyzerResults::GenerateExportFile( const char* file,
                                              DisplayBase display_base,
                                              U32 export_type_user_id )
{
    U64 first_frame_id;
    U64 last_frame_id;
    U64 address;
	char time_str[16];
    char packet_str[16];
    char sa_str[8];
    char type_str[16];
    char addr_str[16];
    char parity_str[8];
    char parityCmd_str[8];
    char bc_str[8];
    char data_str[8];
    bool show_parity = mSettings->mShowParityInReport;
    bool show_buspark = mSettings->mShowBusParkInReport;
    std::stringstream payload;
    std::stringstream ss;
    Frame frame;
	void* f = AnalyzerHelpers::StartFile( file );

    export_type_user_id = export_type_user_id;

	U64 trigger_sample  = mAnalyzer->GetTriggerSample();
	U32 sample_rate     = mAnalyzer->GetSampleRate();

	ss << "Time [s],Packet ID,SSC,SA,Type,Adr,BC,Payload" << std::endl;

	U64 num_packets = GetNumPackets();
	for( U32 i = 0; i < num_packets; i++ )
	{
        // package id
		AnalyzerHelpers::GetNumberString( i, Decimal, 0, packet_str, 16 );

        payload.str( std::string() );
        sprintf_s( sa_str, 8, "" );
        sprintf_s( type_str, 8, "" );
        sprintf_s( addr_str, 8, "" );
        sprintf_s( parity_str, 8, "" );
        sprintf_s( parityCmd_str, 8, "" );
        sprintf_s( bc_str, 8, "" );
        sprintf_s( data_str, 8, "" );
        address = 0xFFFFFFFF;

		GetFramesContainedInPacket( i, &first_frame_id, &last_frame_id );
        for ( U64 j = first_frame_id; j <= last_frame_id; j++ )
        {
    		frame = GetFrame( j );

            switch( frame.mType )
            {
            case RffeSSCField:
                // starting time using SSC as marker
		        AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive,
                                                trigger_sample,
                                                sample_rate,
                                                time_str,
                                                16 );
                break;

            case RffeSAField:
		        AnalyzerHelpers::GetNumberString( frame.mData1, 
                                                  display_base,
                                                  4,
                                                  sa_str,
                                                  8 );
                break;

            case RffeTypeField:
                sprintf_s( type_str, sizeof(type_str), "%s", RffeTypeStringMid[frame.mData1] );
                break;

            case RffeExByteCountField:
		        AnalyzerHelpers::GetNumberString( frame.mData1,
                                                  display_base,
                                                  4,
                                                  bc_str,
                                                  8 );
                break;

            case RffeExLongByteCountField:
		        AnalyzerHelpers::GetNumberString( frame.mData1,
                                                  display_base,
                                                  3,
                                                  bc_str,
                                                  8 );
                break;

            case RffeShortAddressField:
                address = frame.mData1;
                break;

            case RffeAddressField:
                switch( frame.mData2 )
                {
                case RffeAddressHiField:
                    address = (frame.mData1<<8);
                    break;
                case RffeAddressLoField:
                    address |= frame.mData1;
                    break;
                case RffeAddressNormalField:
                default:
                    address = frame.mData1;
                    break;
                }
                break;

            case RffeShortDataField:
		        AnalyzerHelpers::GetNumberString( frame.mData1,
                                                  display_base,
                                                  7,
                                                  data_str,
                                                  8 );
		        payload << "D:" << data_str << " ";
                break;

            case RffeDataField:
		        AnalyzerHelpers::GetNumberString( frame.mData1,
                    display_base,
                    8,
                    data_str,
                    8 );
		        payload << data_str << " ";
                break;

            case RffeParityField:
                if ( ! show_parity ) break;
                if ( frame.mData2 == 0 )
                {
		            AnalyzerHelpers::GetNumberString( frame.mData1,
                        Decimal,
                        1,
                        parity_str,
                        4 );
    		        payload << "P" << parity_str << " ";
                }
                else
                {
		            AnalyzerHelpers::GetNumberString( frame.mData1,
                        Decimal,
                        1,
                        parityCmd_str,
                        4 );
                }
                break;

            case RffeBusParkField:
                if( ! show_buspark ) break;

                payload << "BP ";
                break;

            case RffeErrorCaseField:
            default:
                char number1_str[20];
                char number2_str[20];

		        AnalyzerHelpers::GetNumberString( frame.mData1,
                    Hexadecimal,
                    32, 
                    number1_str,
                    10 );
		        AnalyzerHelpers::GetNumberString( frame.mData2,
                    Hexadecimal,
                    32,
                    number2_str,
                    10 );
		        payload << "E:" << number1_str << " - " << number2_str << " ";
                break;
            }
        }

        ss << time_str << "," << packet_str << ",SSC," << sa_str << "," << type_str;

        if ( address == 0xFFFFFFFF )
        {
            ss << ",,," << payload.str().c_str();
            if ( show_parity ) ss << " P" << parityCmd_str;
        }
        else
        {
    	    AnalyzerHelpers::GetNumberString( address,
                                              display_base,
                                              8,
                                              addr_str,
                                              8 );

            ss << "," << addr_str;
            if ( show_parity )  ss <<" P" << parityCmd_str;
            ss << "," << bc_str << "," << payload.str().c_str();
        }
        ss << std::endl;
        AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
        ss.str( std::string() );

		if( UpdateExportProgressAndCheckForCancel( i, num_packets ) == true )
		{
			AnalyzerHelpers::EndFile( f );
			return;
		}
    }
}

void RFFEAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	//Frame frame = GetFrame( frame_index );
    frame_index = frame_index;
    display_base = display_base;

	ClearResultStrings();
	AddResultString( "not supported yet" );
}

void RFFEAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
    packet_id = packet_id;
    display_base = display_base;

	ClearResultStrings();
	AddResultString( "not supported yet" );
}

void RFFEAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
    transaction_id = transaction_id;
    display_base = display_base;

	ClearResultStrings();
	AddResultString( "not supported yet" );
}