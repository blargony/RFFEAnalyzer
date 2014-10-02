#include "HD44780AnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "HD44780Analyzer.h"
#include "HD44780AnalyzerSettings.h"
#include <iostream>
#include <fstream>

char *LowerASCIITable[]={"NUL","SOH","STX","ETX","EOT","ENQ","ACK","BEL","BS","TAB","LF","VT","FF","CR","SO","SI",
                         "DLE","DC1","DC2","DC3","DC4","NAK","SYN","ETB","CAN","EM","SUB","ESC","FS","GS","RS","US"};

HD44780AnalyzerResults::HD44780AnalyzerResults( HD44780Analyzer* analyzer, HD44780AnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

HD44780AnalyzerResults::~HD44780AnalyzerResults()
{
}


void HD44780AnalyzerResults::HD44780Decode(Frame *AFrame, char *AString, DisplayBase ADisplayBase )
{
  char number_str[128];

  AString[0]=0;
  if (AFrame->mFlags & _BV(MFLAG_RS))
    {
      strcat(AString,"Data");
      if (AFrame->mData1<128)
        {
          strcat(AString," '");
          if (AFrame->mData1<32)
            strcat(AString,LowerASCIITable[AFrame->mData1]);
          else
          if (AFrame->mData1==127)
            strcat(AString,"DEL");
          else
            {
              AString[6]=(U8)AFrame->mData1;
              AString[7]=0;
            }
          strcat(AString,"'");
        }
    }
  else
    {
      if (AFrame->mFlags & _BV(MFLAG_RW))
        {
          //read busy and address
          if (AFrame->mData1 & _BV(LCD_BUSY))
            strcat(AString,"Busy; Addr ");
          else strcat(AString,"Not Busy; Addr ");

          AnalyzerHelpers::GetNumberString( AFrame->mData1 & 127, ADisplayBase, 8, number_str, 128 );
          strcat(AString,number_str);
        }
      else
        {
          //write command
          if (AFrame->mData1 & _BV(LCD_DDRAM))
            {
              strcat(AString,"Set DDRAM Addr ");
              AnalyzerHelpers::GetNumberString( AFrame->mData1 & 127, ADisplayBase, 8, number_str, 128 );
              strcat(AString,number_str);
            }
          else
          if (AFrame->mData1 & _BV(LCD_CGRAM))
            {
              strcat(AString,"Set CGRAM Addr ");
              AnalyzerHelpers::GetNumberString( AFrame->mData1 & 63, ADisplayBase, 8, number_str, 128 );
              strcat(AString,number_str);
            }
          else
          if (AFrame->mData1 & _BV(LCD_FUNCTION))
            {
              strcat(AString,"Function Set ");
              if (AFrame->mData1 & _BV(LCD_FUNCTION_8BIT))
                strcat(AString,"8-bit; ");
              else strcat(AString,"4-bit; ");
              if (AFrame->mData1 & _BV(LCD_FUNCTION_2LINES))
                strcat(AString,"2 Lines; ");
              else strcat(AString,"1 Line; ");
              if (AFrame->mData1 & _BV(LCD_FUNCTION_10DOTS))
                strcat(AString,"5x10");
              else strcat(AString,"5x8");
            }
          else
          if (AFrame->mData1 & _BV(LCD_MOVE))
            {
              if (AFrame->mData1 & _BV(LCD_MOVE_DISP))
                strcat(AString,"Display Shift; ");
              else strcat(AString,"Cursor Move; ");
              if (AFrame->mData1 & _BV(LCD_MOVE_RIGHT))
                strcat(AString,"Right");
              else strcat(AString,"Left");
            }
          else
          if (AFrame->mData1 & _BV(LCD_DISPLAYMODE))
            {
              strcat(AString,"Display ");
              if (AFrame->mData1 & _BV(LCD_DISPLAYMODE_ON))
                strcat(AString,"On; ");
              else strcat(AString,"Off; ");
              if (AFrame->mData1 & _BV(LCD_DISPLAYMODE_CURSOR))
                strcat(AString,"Cursor On; ");
              else strcat(AString,"Cursor Off; ");
              if (AFrame->mData1 & _BV(LCD_DISPLAYMODE_BLINK))
                strcat(AString,"Blink On");
              else strcat(AString,"Blink Off");
            }
          else
          if (AFrame->mData1 & _BV(LCD_ENTRY_MODE))
            {
              strcat(AString,"Entry Mode Set ");
              if (AFrame->mData1 & _BV(LCD_ENTRY_INC))
                strcat(AString,"Increment; ");
              else strcat(AString,"Decrement; ");
              if (AFrame->mData1 & _BV(LCD_ENTRY_SHIFT))
                strcat(AString,"Shift");
              else strcat(AString,"No Shift");
            }
          else
          if (AFrame->mData1 & _BV(LCD_HOME))
            strcat(AString,"Return Home");
          else
          if (AFrame->mData1 & _BV(LCD_CLR))
            strcat(AString,"Clear Display");
        }
    }
}

void HD44780AnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	char s1[256],s2[256],number_str[128];
	Frame frame;

  frame = GetFrame( frame_index );
	ClearResultStrings();
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

  //string 1
  strcpy(s1,number_str);
  AddResultString( s1 );

  //string 2
  if (frame.mFlags & _BV(MFLAG_RW))
    strcpy(s1,"R");
  else strcpy(s1,"W");

  if (frame.mFlags & _BV(MFLAG_RS))
    strcat(s1,"D ");
  else strcat(s1,"C ");
  
  strcat(s1,number_str);
  AddResultString( s1 );

  //string 3
  if (frame.mFlags & _BV(MFLAG_RW))
    strcpy(s1,"Rd ");
  else strcpy(s1,"Wr ");

  if (frame.mFlags & _BV(MFLAG_RS))
    strcat(s1,"Dt ");
  else strcat(s1,"Cm ");
  
  strcat(s1,number_str);
  AddResultString( s1 );

  //string 4
  if (frame.mFlags & _BV(MFLAG_RW))
    strcpy(s1,"Read ");
  else strcpy(s1,"Write ");

  if (frame.mFlags & _BV(MFLAG_RS))
    strcat(s1,"Data ");
  else strcat(s1,"Command ");
  
  strcat(s1,number_str);
  AddResultString( s1 );

  //string 5 (adds to string 4)
  HD44780Decode(&frame, s2, display_base);
  strcat(s1," (");
  strcat(s1,s2);
  strcat(s1,")");
  AddResultString( s1 );
}

void HD44780AnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
  char s1[256];

	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		file_stream << time_str << ",";

    //string 4
    if (frame.mFlags & _BV(MFLAG_RW))
      file_stream << "Read,";
    else file_stream << "Write,";

    if (frame.mFlags & _BV(MFLAG_RS))
      file_stream << "Data,";
    else file_stream << "Command,";

    file_stream << number_str << ",";

    HD44780Decode(&frame, s1, display_base);
    file_stream << s1 << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void HD44780AnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
  char s1[256],s2[256];
	Frame frame;

  frame = GetFrame( frame_index );
	ClearResultStrings();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

  //string 4
  if (frame.mFlags & _BV(MFLAG_RW))
    strcpy(s1,"Read ");
  else strcpy(s1,"Write ");

  if (frame.mFlags & _BV(MFLAG_RS))
    strcat(s1,"Data ");
  else strcat(s1,"Command ");
  
  strcat(s1,number_str);

  //string 5 (adds to string 4)
  HD44780Decode(&frame, s2, display_base);
  strcat(s1," (");
  strcat(s1,s2);
  strcat(s1,")");
  AddResultString( s1 );
}

void HD44780AnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void HD44780AnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

