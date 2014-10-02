#include "BISSAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "BISSAnalyzer.h"
#include "BISSAnalyzerSettings.h"

#include <iostream>
#include <fstream>
BISSAnalyzerResults::BISSAnalyzerResults( BISSAnalyzer* analyzer, BISSAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

BISSAnalyzerResults::~BISSAnalyzerResults()
{
}

//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
//---------------------------------------------------------------------------------------------------------------------------------
{	
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );
			
	char number_str[128];

	switch (frame.mType)
	{
	case 1: //Sensordaten
		if (mSettings->mDatenart == 1)//Benutzerauswahl Sensordaten
		{
			if (channel == mSettings->mSloChannel)
			{
				char result_str0[128];
				char result_str1[128];
				char result_str2[128];
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );

				switch (frame.mFlags)
				{
				case 1:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"DATA [ %s ]", number_str);
					sprintf( result_str2,"Serial Data [ %s ]", number_str);
					break;
				case 2:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"nEnW [ %s ]", number_str);
					sprintf( result_str2,"nError/nWarning [ %s ]", number_str);
					break;
				case 3:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"CRC [ %s ]", number_str);
					sprintf( result_str2,"CRC [ %s ]", number_str);
					break;
				//default:
				//	sprintf( result_str,"Fehler [ %s ]", number_str);
				//	break;
				}

				AddResultString( result_str0 );	
				AddResultString( result_str1 );
				AddResultString( result_str2 );
			}
		}
		break;

	case 2://CDM
		if (mSettings->mDatenart == 0)//Benutzerauswahl Registerdaten
		{		
			if (channel == mSettings->mMaChannel)
			{ 	
				char result_str0[128];
				char result_str1[128];
				char result_str2[128];
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );

				switch (frame.mFlags)
				{
				case 1:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"S,CTS [ %s ]", number_str);
					sprintf( result_str2,"Start,Control Select [ %s ]", number_str);
					break;
				case 2:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"ID [ %s ]", number_str);
					sprintf( result_str2,"Identifier [ %s ]", number_str);
					break;
				case 3:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"ADR [ %s ]", number_str);
					sprintf( result_str2,"Register Address [ %s ]", number_str);
					break;
				case 4:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"CRC [ %s ]", number_str);
					sprintf( result_str2,"CRC [ %s ]", number_str);
					break;
				case 5:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"RW [ %s ]", number_str);
					sprintf( result_str2,"Read/Write [ %s ]", number_str);
					break;
				case 6:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"DATA [ %s ]", number_str);
					sprintf( result_str2,"Register Data [ %s ]", number_str);
					break;
				case 7:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"CRC [ %s ]", number_str);
					sprintf( result_str2,"CRC [ %s ]", number_str);
					break;
				//default:
				//	sprintf( result_str,"Fehler [ %s ]", number_str);
				//	break;
				}

				AddResultString( result_str0 );	
				AddResultString( result_str1 );
				AddResultString( result_str2 );
			}
		}
		break;

	case 3://CDS
		if (mSettings->mDatenart == 0)//Benutzerauswahl Registerdaten
		{
			if (channel == mSettings->mSloChannel)
			{
				char result_str0[128];
				char result_str1[128];
				char result_str2[128];
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );

				switch (frame.mFlags)
				{
				case 1:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"IDL [ %s ]", number_str);
					sprintf( result_str2,"ID Lock Bits [ %s ]", number_str);
					break;
				case 2:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"[ %s ]", number_str);
					sprintf( result_str2,"[ %s ]", number_str);
					break;
				case 3:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"RW [ %s ]", number_str);
					sprintf( result_str2,"Read/Write [ %s ]", number_str);
					break;
				case 4:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"DATA [ %s ]", number_str);
					sprintf( result_str2,"Register Data [ %s ]", number_str);
					break;
				case 5:
					sprintf( result_str0,"[ %s ]", number_str);
					sprintf( result_str1,"CRC [ %s ]", number_str);
					sprintf( result_str2,"CRC [ %s ]", number_str);
					break;
				//default:
				//	sprintf( result_str,"Fehler [ %s ]", number_str);
				//	break;
				}

				AddResultString( result_str0 );
				AddResultString( result_str1 );
				AddResultString( result_str2 );
			}
		}
		break;
	} 
}
//---------------------------------------------------------------------------------------------------------------------------------


//void BISSAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
//{
//	std::ofstream file_stream( file, std::ios::out );
//
//	U64 trigger_sample = mAnalyzer->GetTriggerSample();
//	U32 sample_rate = mAnalyzer->GetSampleRate();
//
//	file_stream << "Time [s],Value" << std::endl;
//
//	U64 num_frames = GetNumFrames();
//	for( U32 i=0; i < num_frames; i++ )
//	{
//		Frame frame = GetFrame( i );
//		
//		char time_str[128];
//		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );
//
//		char number_str[128];
//		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
//		
//		file_stream << time_str << "," << number_str << std::endl;
//
//		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
//		{
//			file_stream.close();
//			return;
//		}
//	}
//
//	file_stream.close();
//}


//---------------------------------------------------------------------------------------------------------------------------------
void BISSAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
//---------------------------------------------------------------------------------------------------------------------------------
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();
	bool bEndFrame = false;
	bool bNewFrame = true;
	bool bSeqCds = false;
	bool bSeqCdm = false;

	U64 num_frames = GetNumFrames();
	
	if (mSettings->mDatenart == 0)//Benutzerauswahl Registerdaten
		file_stream << "Time[s],CDM_S/CTS,CDM_ID,CDM_ADR,CDM_CRCADR,CDM_RW,CDM_DATA,CDM_CRCDATA,CDS_IDL,CDS_0,CDS_RW,CDS_DATA,CDS_CRCDATA" << std::endl;
	else
		file_stream << "Time[s],DATA,ERR-WARN,CRC" << std::endl;

	//file_stream << "" << std::endl;	

	
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );		

		char number_str[128];
		char time_str[128];

		if (bNewFrame == true)
		{			
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );
			file_stream << time_str << ",";
			bNewFrame = false;	
		}

		switch (frame.mType)
		{
		case 1: //Sensordaten
			switch (frame.mFlags)
			{
			case 1://DATA
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				break;
			case 2://ERR-WARN
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				break;
			case 3://CRC
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				bEndFrame = true;
				bNewFrame = true;
				break;
			}
			break;

		case 2://CDM			
			switch (frame.mFlags)
			{
			case 1://S,CTS
				char result_str[128];
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, result_str, 128 );
				sprintf (number_str, "%s", result_str); 

				bSeqCds = false;
				bSeqCdm = false;
				break;

			case 2://ID
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				break;

			case 3://ADR
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				break;

			case 4://CRC ADR
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );	
				break;

			case 5://RW
				if (frame.mData1 == 0x2)//Read
				{
					char result_str[128];
					AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, result_str, 128 );
					sprintf (number_str, "%s,,", result_str);
					bEndFrame = true;
					bNewFrame = true;
				}
				else
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				break;

			case 6://DATA
					if (bSeqCdm)
				{
					char result_str[128];
					AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, result_str, 128 );
					sprintf (number_str, ",,,,,%s", result_str); 
				}
					else
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				break;

			case 7://CRC DATA
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );	
				bEndFrame = true;
				bNewFrame = true;
				bSeqCdm = true;
				break;
			}
			break;

		case 3://CDS

			switch (frame.mFlags)
			{
			case 1://IDL
				char result_str[128];
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, result_str, 128 );
				sprintf (number_str, ",,,,,,,%s", result_str); 
				break;

			case 2://0
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				break;

			case 3://RW
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				break;

			case 4://DATA
				if (bSeqCds)
				{
					char result_str[128];
					AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, result_str, 128 );
					sprintf (number_str, ",,,,,,,,,,%s", result_str); 
				}
				else
					AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				break;

			case 5://CRC
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, frame.mData2, number_str, 128 );
				bEndFrame = true;
				bNewFrame = true;
				bSeqCds = true;
				break;
			}
			break;
		}
		
		if (bEndFrame == true)
		{
			file_stream << number_str << std::endl;
			bEndFrame = false;			
		}
		else		
			file_stream << number_str << ",";

	
		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}
//---------------------------------------------------------------------------------------------------------------------------------


void BISSAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	//Frame frame = GetFrame( frame_index );
	//ClearResultStrings();

	//char number_str[128];
	//AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	//AddResultString( number_str );
}

void BISSAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void BISSAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}