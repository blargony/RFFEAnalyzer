#include "PS2KeyboardAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "PS2KeyboardAnalyzer.h"
#include "PS2KeyboardAnalyzerSettings.h"
#include "PS2KeyboardAnalyzerScanCodes.h"
#include <iostream>
#include <fstream>
#include <sstream>

PS2KeyboardAnalyzerResults::PS2KeyboardAnalyzerResults( PS2KeyboardAnalyzer* analyzer, PS2KeyboardAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{

}

PS2KeyboardAnalyzerResults::~PS2KeyboardAnalyzerResults()
{
}

void PS2KeyboardAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );
	
	char device_str[16];
	if(frame.mFlags&TX_HOST_TO_DEVICE)
	{
		sprintf(device_str,"Host");

		char num_str[128];
		AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,32);

		char cmd_str[64];
		GetCommandName(cmd_str, frame.mData1, mSettings->mDeviceType);

		char result_str[255];
		
		//at furthest zoom, show raw hex
		AddResultString(num_str);

		//next add type
		sprintf(result_str,"%s %s",cmd_str,num_str);
		AddResultString(result_str);

		//add key str
		sprintf(result_str,"%s: %s (%s)",device_str,cmd_str,num_str);
		AddResultString(result_str);

	}
	else
	{
		if(mSettings->mDeviceType==0)
		{
			sprintf(device_str,"Keyboard");
			
			char num_str[128];
			AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,32);
			
			char tmp_str[128];
			char codetype[32];

			char key_str[64];
			GetKeyName(key_str, frame.mData1, frame.mFlags&EXTENDED_KEY);
			

			if(frame.mFlags&DATA_FRAME)
			{
				if(frame.mData2&ACK_FRAME)
				{
					sprintf(tmp_str,"%s",num_str);
					sprintf(codetype,"ACK");
					sprintf(key_str,"");
				}
				else if (frame.mData2&ECHO_FRAME)
				{
					sprintf(tmp_str,"%s",num_str);
					sprintf(codetype,"ECHO");

					sprintf(key_str,"");
				}
				else if (frame.mData2&BAT_FRAME)
				{
					sprintf(tmp_str,"%s",num_str);
					sprintf(codetype,"BAT");

                    sprintf(key_str,"Power-On Self Test Successfull");
				}
			}
			else if(frame.mFlags&ERROR_FRAME)
			{
				sprintf(tmp_str,"%s",num_str);
				sprintf(codetype,"ERROR");
				sprintf(key_str,"INVALID CODE");
			}
			else if(frame.mFlags&PAUSE_BREAK)
			{
				sprintf(tmp_str,"0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77");
				sprintf(codetype,"");
				sprintf(key_str,"[PAUSE/BREAK]");
			}
			else if (frame.mFlags&PRINT_SCREEN && frame.mFlags&BREAK_CODE)
			{
				sprintf(tmp_str,"0xE0, 0xF0, 0x7C, 0xE0, 0xF0, 0x12");
				sprintf(codetype,"BREAK");
				sprintf(key_str,"[PRINT SCREEN]");
			}
			else if (frame.mFlags&PRINT_SCREEN)
			{
				sprintf(tmp_str,"0xE0, 0x12, 0xE0, 0x7C");
				sprintf(codetype,"MAKE");
				sprintf(key_str,"[PRINT SCREEN]");
			}
			else if (frame.mFlags&EXTENDED_KEY && frame.mFlags&BREAK_CODE)
			{
				sprintf(tmp_str,"0xE0, 0xF0, %s",num_str);
				sprintf(codetype,"BREAK");
			}
			else if (frame.mFlags&EXTENDED_KEY)
			{
				sprintf(tmp_str,"0xE0, %s",num_str);
				sprintf(codetype,"MAKE");
			}
			else if (frame.mFlags&BREAK_CODE)
			{
				sprintf(tmp_str,"0xF0, %s",num_str);
				sprintf(codetype,"BREAK");
			}
			else
			{
				sprintf(tmp_str,"%s",num_str);
				sprintf(codetype,"MAKE");
			}
			
			sprintf(num_str,"%s",tmp_str);

			char result_str[255];
		
			//at furthest zoom, show raw hex
			AddResultString(num_str);

			//next add type
			sprintf(result_str,"%s %s",codetype,num_str);
			AddResultString(result_str);

			//add key str
			sprintf(result_str,"%s %s (%s)",codetype,key_str,num_str);
			AddResultString(result_str);

			//at next zoom show originator and raw hex
			sprintf(result_str,"%s: %s %s (%s)",device_str,codetype,key_str,num_str);
			AddResultString(result_str);
		}
		else
		{
			sprintf(device_str,"Mouse");

			char num_str[128];
		
			char tmp_str[256];
			char codetype[32];

			char key_str[512];
			GetKeyName(key_str, frame.mData1, frame.mFlags&EXTENDED_KEY);

			if(frame.mFlags&DATA_FRAME)
			{
				if(frame.mData2&ACK_FRAME)
				{
					AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,128);
					sprintf(tmp_str,"%s",num_str);
					sprintf(codetype,"ACK");
					sprintf(key_str,"");
				}
				else if (frame.mData2&BAT_FRAME)
				{
					AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,128);
					sprintf(tmp_str,"%s",num_str);
					sprintf(codetype,"BAT");

                    sprintf(key_str,"Power-On Self Test Successfull");
				}
			}
			else if(frame.mFlags&MOVEMENT_FRAME)
			{
					AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,32,num_str,128);
					sprintf(tmp_str,"%s",num_str);
					sprintf(codetype,"MOVEMENT");
					
					U8 YOverflow = (frame.mData1&0x80) >> 7;
					U8 XOverflow = (frame.mData1&0x40) >> 6;
					U8 YSignBit = (frame.mData1&0x20) >> 5;
					U8 XSignBit = (frame.mData1&0x10) >> 4;
					U8 MiddleButton = (frame.mData1&0x04) >> 2;
					U8 RightButton = (frame.mData1&0x02) >> 1;
					U8 LeftButton = frame.mData1&0x01;
					
					U8 Xmovement = (frame.mData1&0xFF00) >> 8;
					U8 Ymovement = (frame.mData1&0xFF0000) >> 16;

					if(mSettings->mDeviceType==1)
						sprintf(key_str,"Y Overflow: %d, X Overflow: %d, Y Sign Bit: %d, X Sign Bit: %d, Middle Btn: %d, Right Btn: %d, Left Btn: %d, deltaX: %d, deltaY: %d", YOverflow, XOverflow, YSignBit, XSignBit, MiddleButton, RightButton, LeftButton, Xmovement, Ymovement);
					else
					{
						
						U8 FifthButton = (frame.mData1&0x20000000) >> 29;
						U8 FourthButton = (frame.mData1&0x1000000) >> 28;
						U8 Zmovement = (frame.mData1&0x0F000000) >> 24;

						sprintf(key_str,"Y Overflow: %d, X Overflow: %d, Y Sign Bit: %d, X Sign Bit: %d, Middle Btn: %d, Right Btn: %d, Left Btn: %d, 4th Btn: %d, 5th Btn: %d, deltaX: %d, deltaY: %d, deltaZ: %d", YOverflow, XOverflow, YSignBit, XSignBit, MiddleButton, RightButton, LeftButton, FourthButton, FifthButton, Xmovement, Ymovement, Zmovement);
					}
			}
			else
			{	
					AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,128);
					sprintf(tmp_str,"%s",num_str);
					sprintf(codetype,"DATA");

					sprintf(key_str,"");
			}
			sprintf(num_str,"%s",tmp_str);

			char result_str[255];
		
			//at furthest zoom, show raw hex
			AddResultString(num_str);

			//next add type
			sprintf(result_str,"%s %s",codetype,num_str);
			AddResultString(result_str);

			//add key str
			sprintf(result_str,"%s %s (%s)",codetype,key_str,num_str);
			AddResultString(result_str);

			//at next zoom show originator and raw hex
			sprintf(result_str,"%s: %s %s (%s)",device_str,codetype,key_str,num_str);
			AddResultString(result_str);
		}
	}
}

void PS2KeyboardAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	if(export_type_user_id==0)
	{
		if(mSettings->mDeviceType==0)
		{
			//keys as text file, no output for mouse
			U64 num_frames = GetNumFrames();
			for( U32 i=0; i < num_frames; i++ )
			{
				Frame frame = GetFrame( i );

					char key_str[64];

					//set the key based on the data, then if the context of the transmissions means its not correct, just make it an empty string
					GetKey(key_str, frame.mData1, frame.mFlags&EXTENDED_KEY);
					
					//context checking, only allows make codes to generate a character in the file
					if(frame.mFlags&DATA_FRAME)
					{
						if(frame.mData2&ACK_FRAME)
							sprintf(key_str,"");
						else if (frame.mData2&ECHO_FRAME)
							sprintf(key_str,"");
						else if (frame.mData2&BAT_FRAME)
							sprintf(key_str,"");
					}
					else if(frame.mFlags&ERROR_FRAME)
						sprintf(key_str,"");
					else if(frame.mFlags&PAUSE_BREAK)
						sprintf(key_str,"[PAUSE/BREAK]");
					else if (frame.mFlags&PRINT_SCREEN && frame.mFlags&BREAK_CODE)
						sprintf(key_str,"");
					else if (frame.mFlags&PRINT_SCREEN)
						sprintf(key_str,"[PRINT SCREEN]");
					else if (frame.mFlags&EXTENDED_KEY && frame.mFlags&BREAK_CODE)
						sprintf(key_str,"");
					else if (frame.mFlags&BREAK_CODE)
						sprintf(key_str,"");
					
					file_stream << key_str;

				if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
				{
					file_stream.close();
					return;
				}
			}
		}
		else
		{
			//no output for mouse, but don't want the program to crash so put this here for stability
			U64 num_frames = GetNumFrames();
			for( U32 i=0; i < num_frames; i++ )
			{
				Frame frame = GetFrame( i );

				if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
				{
					file_stream.close();
					return;
				}

			}
		}
	}
	else
	{
		//csv data log file, mouse & keyboard support
		
		//print column titles
		file_stream << "Time [s], Originator, Type, Details, Raw Data" << std::endl;

		U64 num_frames = GetNumFrames();
		for( U32 i=0; i < num_frames; i++ )
		{
			Frame frame = GetFrame( i );

			char time_str[128];
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive,trigger_sample, sample_rate, time_str, 128 );

			char device_str[16];
			if(frame.mFlags&TX_HOST_TO_DEVICE)
			{
				sprintf(device_str,"Host");

				char num_str[128];
				AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,32);

				char cmd_str[64];
				GetCommandName(cmd_str, frame.mData1, mSettings->mDeviceType);

				char result_str[255];

				//add key str
				sprintf(result_str,"%s: %s (%s)",device_str,cmd_str,num_str);

				file_stream << time_str << "," << device_str << ",," << cmd_str << "," << num_str << std::endl;


			}
			else
			{
				if(mSettings->mDeviceType==0)
				{
					sprintf(device_str,"Keyboard");
					
					char num_str[128];
					AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,32);
					
					char tmp_str[128];
					char codetype[32];

					char key_str[64];
					GetKeyName(key_str, frame.mData1, frame.mFlags&EXTENDED_KEY);
					

					if(frame.mFlags&DATA_FRAME)
					{
						if(frame.mData2&ACK_FRAME)
						{
							sprintf(tmp_str,"%s",num_str);
							sprintf(codetype,"ACK");
							sprintf(key_str,"");
						}
						else if (frame.mData2&ECHO_FRAME)
						{
							sprintf(tmp_str,"%s",num_str);
							sprintf(codetype,"ECHO");

							sprintf(key_str,"");
						}
						else if (frame.mData2&BAT_FRAME)
						{
							sprintf(tmp_str,"%s",num_str);
							sprintf(codetype,"BAT");

                            sprintf(key_str,"Power-On Self Test Successfull");
						}
					}
					else if(frame.mFlags&ERROR_FRAME)
					{
						sprintf(tmp_str,"%s",num_str);
						sprintf(codetype,"ERROR");
						sprintf(key_str,"INVALID CODE");
					}
					else if(frame.mFlags&PAUSE_BREAK)
					{
						sprintf(tmp_str,"0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77");
						sprintf(codetype,"");
						sprintf(key_str,"[PAUSE/BREAK]");
					}
					else if (frame.mFlags&PRINT_SCREEN && frame.mFlags&BREAK_CODE)
					{
						sprintf(tmp_str,"0xE0, 0xF0, 0x7C, 0xE0, 0xF0, 0x12");
						sprintf(codetype,"BREAK");
						sprintf(key_str,"[PRINT SCREEN]");
					}
					else if (frame.mFlags&PRINT_SCREEN)
					{
						sprintf(tmp_str,"0xE0, 0x12, 0xE0, 0x7C");
						sprintf(codetype,"MAKE");
						sprintf(key_str,"[PRINT SCREEN]");
					}
					else if (frame.mFlags&EXTENDED_KEY && frame.mFlags&BREAK_CODE)
					{
						sprintf(tmp_str,"0xE0, 0xF0, %s",num_str);
						sprintf(codetype,"BREAK");
					}
					else if (frame.mFlags&EXTENDED_KEY)
					{
						sprintf(tmp_str,"0xE0, %s",num_str);
						sprintf(codetype,"MAKE");
					}
					else if (frame.mFlags&BREAK_CODE)
					{
						sprintf(tmp_str,"0xF0, %s",num_str);
						sprintf(codetype,"BREAK");
					}
					else
					{
						sprintf(tmp_str,"%s",num_str);
						sprintf(codetype,"MAKE");
					}
					
					sprintf(num_str,"%s",tmp_str);
					
					//write the info to the file
					file_stream << time_str << "," << device_str << "," << codetype << "," << key_str <<"," << num_str << std::endl;

				}
				else
				{
					sprintf(device_str,"Mouse");

					char num_str[128];
				
					char tmp_str[256];
					char codetype[32];

					char key_str[512];
					GetKeyName(key_str, frame.mData1, frame.mFlags&EXTENDED_KEY);

					if(frame.mFlags&DATA_FRAME)
					{
						if(frame.mData2&ACK_FRAME)
						{
							AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,128);
							sprintf(tmp_str,"%s",num_str);
							sprintf(codetype,"ACK");
							sprintf(key_str,"");
						}
						else if (frame.mData2&BAT_FRAME)
						{
							AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,128);
							sprintf(tmp_str,"%s",num_str);
							sprintf(codetype,"BAT");

                            sprintf(key_str,"Power-On Self Test Successfull");
						}
					}
					else if(frame.mFlags&MOVEMENT_FRAME)
					{
							AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,32,num_str,128);
							sprintf(tmp_str,"%s",num_str);
							sprintf(codetype,"MOVEMENT");
							
							U8 YOverflow = (frame.mData1&0x80) >> 7;
							U8 XOverflow = (frame.mData1&0x40) >> 6;
							U8 YSignBit = (frame.mData1&0x20) >> 5;
							U8 XSignBit = (frame.mData1&0x10) >> 4;
							U8 MiddleButton = (frame.mData1&0x04) >> 2;
							U8 RightButton = (frame.mData1&0x02) >> 1;
							U8 LeftButton = frame.mData1&0x01;
							
							U8 Xmovement = (frame.mData1&0xFF00) >> 8;
							U8 Ymovement = (frame.mData1&0xFF0000) >> 16;

							if(mSettings->mDeviceType==1)
								sprintf(key_str,"Y Overflow: %d, X Overflow: %d, Y Sign Bit: %d, X Sign Bit: %d, Middle Btn: %d, Right Btn: %d, Left Btn: %d, deltaX: %d, deltaY: %d", YOverflow, XOverflow, YSignBit, XSignBit, MiddleButton, RightButton, LeftButton, Xmovement, Ymovement);
							else
							{
								
								U8 FifthButton = (frame.mData1&0x20000000) >> 29;
								U8 FourthButton = (frame.mData1&0x1000000) >> 28;
								U8 Zmovement = (frame.mData1&0x0F000000) >> 24;

								sprintf(key_str,"Y Overflow: %d, X Overflow: %d, Y Sign Bit: %d, X Sign Bit: %d, Middle Btn: %d, Right Btn: %d, Left Btn: %d, 4th Btn: %d, 5th Btn: %d, deltaX: %d, deltaY: %d, deltaZ: %d", YOverflow, XOverflow, YSignBit, XSignBit, MiddleButton, RightButton, LeftButton, FourthButton, FifthButton, Xmovement, Ymovement, Zmovement);
							}
					}
					else
					{	
							AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,128);
							sprintf(tmp_str,"%s",num_str);
							sprintf(codetype,"DATA");

							sprintf(key_str,"");
					}
					sprintf(num_str,"%s",tmp_str);

					file_stream << time_str << "," << device_str << "," << codetype << "," << key_str <<"," << num_str << std::endl;
				}
			}

			if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
			{
				file_stream.close();
				return;
			}
		}
	}

	file_stream.close();
}
 
void PS2KeyboardAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
    ClearTabularText();
	Frame frame = GetFrame( frame_index );
	
    char device_str[16];
    if(frame.mFlags&TX_HOST_TO_DEVICE)
    {
        sprintf(device_str,"Host");

        char num_str[128];
        AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,32);

        char cmd_str[64];
        GetCommandName(cmd_str, frame.mData1, mSettings->mDeviceType);

        char result_str[255];

        sprintf(result_str,"%s: %s (%s)",device_str,cmd_str,num_str);
        AddTabularText(result_str);

    }
    else
    {
        if(mSettings->mDeviceType==0)
        {
            sprintf(device_str,"Keyboard");

            char num_str[128];
            AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,32);

            char tmp_str[128];
            char codetype[32];

            char key_str[64];
            GetKeyName(key_str, frame.mData1, frame.mFlags&EXTENDED_KEY);


            if(frame.mFlags&DATA_FRAME)
            {
                if(frame.mData2&ACK_FRAME)
                {
                    sprintf(tmp_str,"%s",num_str);
                    sprintf(codetype,"ACK");
                    sprintf(key_str,"");
                }
                else if (frame.mData2&ECHO_FRAME)
                {
                    sprintf(tmp_str,"%s",num_str);
                    sprintf(codetype,"ECHO");

                    sprintf(key_str,"");
                }
                else if (frame.mData2&BAT_FRAME)
                {
                    sprintf(tmp_str,"%s",num_str);
                    sprintf(codetype,"BAT");

                    sprintf(key_str,"Power-On Self Test Successfull");
                }
            }
            else if(frame.mFlags&ERROR_FRAME)
            {
                sprintf(tmp_str,"%s",num_str);
                sprintf(codetype,"ERROR");
                sprintf(key_str,"INVALID CODE");
            }
            else if(frame.mFlags&PAUSE_BREAK)
            {
                sprintf(tmp_str,"0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77");
                sprintf(codetype,"");
                sprintf(key_str,"[PAUSE/BREAK]");
            }
            else if (frame.mFlags&PRINT_SCREEN && frame.mFlags&BREAK_CODE)
            {
                sprintf(tmp_str,"0xE0, 0xF0, 0x7C, 0xE0, 0xF0, 0x12");
                sprintf(codetype,"BREAK");
                sprintf(key_str,"[PRINT SCREEN]");
            }
            else if (frame.mFlags&PRINT_SCREEN)
            {
                sprintf(tmp_str,"0xE0, 0x12, 0xE0, 0x7C");
                sprintf(codetype,"MAKE");
                sprintf(key_str,"[PRINT SCREEN]");
            }
            else if (frame.mFlags&EXTENDED_KEY && frame.mFlags&BREAK_CODE)
            {
                sprintf(tmp_str,"0xE0, 0xF0, %s",num_str);
                sprintf(codetype,"BREAK");
            }
            else if (frame.mFlags&EXTENDED_KEY)
            {
                sprintf(tmp_str,"0xE0, %s",num_str);
                sprintf(codetype,"MAKE");
            }
            else if (frame.mFlags&BREAK_CODE)
            {
                sprintf(tmp_str,"0xF0, %s",num_str);
                sprintf(codetype,"BREAK");
            }
            else
            {
                sprintf(tmp_str,"%s",num_str);
                sprintf(codetype,"MAKE");
            }

            sprintf(num_str,"%s",tmp_str);

            char result_str[255];

            //at next zoom show originator and raw hex
            sprintf(result_str,"%s: %s %s (%s)",device_str,codetype,key_str,num_str);
            AddTabularText(result_str);
        }
        else
        {
            sprintf(device_str,"Mouse");

            char num_str[128];

            char tmp_str[256];
            char codetype[32];

            char key_str[512];
            GetKeyName(key_str, frame.mData1, frame.mFlags&EXTENDED_KEY);

            if(frame.mFlags&DATA_FRAME)
            {
                if(frame.mData2&ACK_FRAME)
                {
                    AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,128);
                    sprintf(tmp_str,"%s",num_str);
                    sprintf(codetype,"ACK");
                    sprintf(key_str,"");
                }
                else if (frame.mData2&BAT_FRAME)
                {
                    AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,128);
                    sprintf(tmp_str,"%s",num_str);
                    sprintf(codetype,"BAT");

                    sprintf(key_str,"Power-On Self Test Successfull");
                }
            }
            else if(frame.mFlags&MOVEMENT_FRAME)
            {
                    AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,32,num_str,128);
                    sprintf(tmp_str,"%s",num_str);
                    sprintf(codetype,"MOVEMENT");

                    U8 YOverflow = (frame.mData1&0x80) >> 7;
                    U8 XOverflow = (frame.mData1&0x40) >> 6;
                    U8 YSignBit = (frame.mData1&0x20) >> 5;
                    U8 XSignBit = (frame.mData1&0x10) >> 4;
                    U8 MiddleButton = (frame.mData1&0x04) >> 2;
                    U8 RightButton = (frame.mData1&0x02) >> 1;
                    U8 LeftButton = frame.mData1&0x01;

                    U8 Xmovement = (frame.mData1&0xFF00) >> 8;
                    U8 Ymovement = (frame.mData1&0xFF0000) >> 16;

                    if(mSettings->mDeviceType==1)
                        sprintf(key_str,"Y Overflow: %d, X Overflow: %d, Y Sign Bit: %d, X Sign Bit: %d, Middle Btn: %d, Right Btn: %d, Left Btn: %d, deltaX: %d, deltaY: %d", YOverflow, XOverflow, YSignBit, XSignBit, MiddleButton, RightButton, LeftButton, Xmovement, Ymovement);
                    else
                    {

                        U8 FifthButton = (frame.mData1&0x20000000) >> 29;
                        U8 FourthButton = (frame.mData1&0x1000000) >> 28;
                        U8 Zmovement = (frame.mData1&0x0F000000) >> 24;

                        sprintf(key_str,"Y Overflow: %d, X Overflow: %d, Y Sign Bit: %d, X Sign Bit: %d, Middle Btn: %d, Right Btn: %d, Left Btn: %d, 4th Btn: %d, 5th Btn: %d, deltaX: %d, deltaY: %d, deltaZ: %d", YOverflow, XOverflow, YSignBit, XSignBit, MiddleButton, RightButton, LeftButton, FourthButton, FifthButton, Xmovement, Ymovement, Zmovement);
                    }
            }
            else
            {
                    AnalyzerHelpers::GetNumberString(frame.mData1,Hexadecimal,8,num_str,128);
                    sprintf(tmp_str,"%s",num_str);
                    sprintf(codetype,"DATA");

                    sprintf(key_str,"");
            }
            sprintf(num_str,"%s",tmp_str);

            char result_str[255];
            sprintf(result_str,"%s: %s %s (%s)",device_str,codetype,key_str,num_str);
            AddTabularText(result_str);
        }
    }
}

void PS2KeyboardAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void PS2KeyboardAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void PS2KeyboardAnalyzerResults::GetKeyName(char returnstr[], U64 code, int keyset)
{
	if(keyset==0)
	{
		switch(code)
		{
			case KEY_ESC:
				sprintf(returnstr, "[ESC]");
				break;
			case KEY_F1:
				sprintf(returnstr, "[F1]");
				break;
			case KEY_F2:
				sprintf(returnstr, "[F2]");
				break;
			case KEY_F3:
				sprintf(returnstr, "[F3]");
				break;
			case KEY_F4:
				sprintf(returnstr, "[F4]");
				break;
			case KEY_F5:
				sprintf(returnstr, "[F5]");
				break;
			case KEY_F6:
				sprintf(returnstr, "[F6]");
				break;
			case KEY_F7:
				sprintf(returnstr, "[F7]");
				break;
			case KEY_F8:
				sprintf(returnstr, "[F8]");
				break;
			case KEY_F9:
				sprintf(returnstr, "[F9]");
				break;
			case KEY_F10:
				sprintf(returnstr, "[F10]");
				break;
			case KEY_F11:
				sprintf(returnstr, "[F11]");
				break;
			case KEY_F12:
				sprintf(returnstr, "[F12]");
				break;
			case KEY_SCROLLLOCK:
				sprintf(returnstr, "[SCROLL LOCK]");
				break;
			case KEY_ACCENT:
				sprintf(returnstr, "'`'");
				break;
			case KEY_MINUS:
				sprintf(returnstr, "'-'");
				break;
			case KEY_EQUAL:
				sprintf(returnstr, "'='");
				break;
			case KEY_BACKSPACE:
				sprintf(returnstr, "[BACK SPACE]");
				break;
			case KEY_TAB:
				sprintf(returnstr, "[TAB]");
				break;
			case KEY_LBRACE:
				sprintf(returnstr, "'['");
				break;
			case KEY_RBRACE:
				sprintf(returnstr, "']'");
				break;
			case KEY_BSLASH:
				sprintf(returnstr, "'\\'");
				break;
			case KEY_CAPSLOCK:
				sprintf(returnstr, "[CAPS]");
				break;
			case KEY_COLON:
				sprintf(returnstr, "';'");
				break;
			case KEY_QUOTE:
				sprintf(returnstr, "'''");
				break;
			case KEY_ENTER:
				sprintf(returnstr, "[ENTER]");
				break;
			case KEY_COMMA:
				sprintf(returnstr, "','");
				break;
			case KEY_PERIOD:
				sprintf(returnstr, "'.'");
				break;
			case KEY_FSLASH:
				sprintf(returnstr, "'/'");
				break;
			case KEY_LSHIFT:
				sprintf(returnstr, "[SHIFT (LEFT)]");
				break;
			case KEY_RSHIFT: 
				sprintf(returnstr, "[SHIFT (RIGHT)]");
				break;
			case KEY_LCONTROL: 
				sprintf(returnstr, "[CONTROL (LEFT)]");
				break;
			case KEY_LALT :
				sprintf(returnstr, "[ALT (LEFT)]");
				break;
			case KEY_SPACE :
				sprintf(returnstr, "[SPACE]");
				break;
			case KEY_ASTERISK :
				sprintf(returnstr, "'*'");
				break;
			case KEY_NUMLOCK :
				sprintf(returnstr, "[NUM LOCK]");
				break;
			case KEY_NUMPAD_1 :
				sprintf(returnstr, "'1' (NUMPAD)");
				break;
			case KEY_NUMPAD_2 :
				sprintf(returnstr, "'2' (NUMPAD)");
				break;
			case KEY_NUMPAD_3 :
				sprintf(returnstr, "'3' (NUMPAD)");
				break;
			case KEY_NUMPAD_4 :
				sprintf(returnstr, "'4' (NUMPAD)");
				break;
			case KEY_NUMPAD_5 :
				sprintf(returnstr, "'5' (NUMPAD)");
				break;
			case KEY_NUMPAD_6 :
				sprintf(returnstr, "'6' (NUMPAD)");
				break;
			case KEY_NUMPAD_7 :
				sprintf(returnstr, "'7' (NUMPAD)");
				break;
			case KEY_NUMPAD_8 :
				sprintf(returnstr, "'8' (NUMPAD)");
				break;
			case KEY_NUMPAD_9 :
				sprintf(returnstr, "'9' (NUMPAD)");
				break;
			case KEY_NUMPAD_0 :
				sprintf(returnstr, "'0' (NUMPAD)");
				break;
			case KEY_NUMPAD_DECIMAL :
				sprintf(returnstr, "'.' (NUMPAD)");
				break;
			case KEY_NUMPAD_PLUS :
				sprintf(returnstr, "'+'");
				break;
			case KEY_NUMPAD_MINUS :
				sprintf(returnstr, "'-' (NUMPAD)");
				break;
			case KEY_Q :
				sprintf(returnstr, "'Q'");
				break;
			case KEY_W:
				sprintf(returnstr, "'W'");
				break;
			case KEY_E :
				sprintf(returnstr, "'E'");
				break;
			case KEY_R :
				sprintf(returnstr, "'R'");
				break;
			case KEY_T :
				sprintf(returnstr, "'T'");
				break;
			case KEY_Y :
				sprintf(returnstr, "'Y'");
				break;
			case KEY_U :
				sprintf(returnstr, "'U'");
				break;
			case KEY_I :
				sprintf(returnstr, "'I'");
				break;
			case KEY_O :
				sprintf(returnstr, "'O'");
				break;
			case KEY_P :
				sprintf(returnstr, "'P'");
				break;
			case KEY_A :
				sprintf(returnstr, "'A'");
				break; 
			case KEY_S :
				sprintf(returnstr, "'S'");
				break;
			case KEY_D :
				sprintf(returnstr, "'D'");
				break;
			case KEY_F :
				sprintf(returnstr, "'F'");
				break;
			case KEY_G :
				sprintf(returnstr, "'G'");
				break;
			case KEY_H :
				sprintf(returnstr, "'H'");
				break;
			case KEY_J :
				sprintf(returnstr, "'J'");
				break;
			case KEY_K:
				sprintf(returnstr, "'K'");
				break;
			case KEY_L :
				sprintf(returnstr, "'L'");
				break;
			case KEY_Z :
				sprintf(returnstr, "'Z'");
				break;
			case KEY_X :
				sprintf(returnstr, "'X'");
				break;
			case KEY_C :
				sprintf(returnstr, "'C'");
				break;
			case KEY_V :
				sprintf(returnstr, "'V'");
				break;
			case KEY_B :
				sprintf(returnstr, "'B'");
				break;
			case KEY_N :
				sprintf(returnstr, "'N'");
				break;
			case KEY_M :
				sprintf(returnstr, "'M'");
				break;
			case KEY_1 :
				sprintf(returnstr, "'1'");
				break;
			case KEY_2 :
				sprintf(returnstr, "'2'");
				break;
			case KEY_3 :
				sprintf(returnstr, "'3'");
				break;
			case KEY_4 :
				sprintf(returnstr, "'4'");
				break;
			case KEY_5 :
				sprintf(returnstr, "'5'");
				break;
			case KEY_6 :
				sprintf(returnstr, "'6'");
				break;
			case KEY_7 :
				sprintf(returnstr, "'7'");
				break;
			case KEY_8 :
				sprintf(returnstr, "'8'");
				break;
			case KEY_9 :
				sprintf(returnstr, "'9'");
				break;
			case KEY_0 :
				sprintf(returnstr, "'0'");
				break;
			default:
				sprintf(returnstr, "Unrecognized Key");
		}
	}
	else
	{
		switch(code)
		{
			case EXTENDED_RALT:
				sprintf(returnstr, "[ALT (RIGHT)]");
				break;
			case EXTENDED_RWINDOWS:
				sprintf(returnstr, "[WINDOWS (RIGHT)]");
				break;
			case EXTENDED_MENUS:
				sprintf(returnstr, "[MENU]");
				break;
			case EXTENDED_INSERT:
				sprintf(returnstr, "[INSERT]");
				break;
			case EXTENDED_HOME:
				sprintf(returnstr, "[HOME]");
				break;
			case EXTENDED_PAGEUP:
				sprintf(returnstr, "[PAGE UP]");
				break;
			case EXTENDED_DELETE:
				sprintf(returnstr, "[DELETE]");
				break;
			case EXTENDED_END:
				sprintf(returnstr, "[END]");
				break;
			case EXTENDED_PAGEDOWN:
				sprintf(returnstr, "[PAGE DOWN]");
				break;
			case EXTENDED_UPARROW:
				sprintf(returnstr, "[ARROW UP]");
				break;
			case EXTENDED_LEFTARROW:
				sprintf(returnstr, "[ARROW LEFT]");
				break;
			case EXTENDED_DOWNARROW:
				sprintf(returnstr, "[ARROW DOWN]");
				break;
			case EXTENDED_RIGHTARROW:
				sprintf(returnstr, "[ARROW RIGHT]");
				break;
			case EXTENDED_RCONTROL:
				sprintf(returnstr, "[CONTROL (RIGHT)]");
				break;
			case EXTENDED_LWINDOWS:
				sprintf(returnstr, "[WINDOWS (LEFT)]");
				break;
			case EXTENDED_NUMPAD_ENTER:
				sprintf(returnstr, "[ENTER (NUMPAD)]");
				break;
			case EXTENDED_FSLASH:
				sprintf(returnstr, "'\\'");
				break;
			case EXTENDED_POWER:
				sprintf(returnstr, "[POWER]");
				break;
			case EXTENDED_SLEEP:
				sprintf(returnstr, "[SLEEP]");
				break;
			case EXTENDED_WAKE:
				sprintf(returnstr, "[WAKE]");
				break;
			case EXTENDED_NEXTTRACK:
				sprintf(returnstr, "[NEXT TRACK (Multimedia)]");
				break;
			case EXTENDED_PREVIOUSTRACK:
				sprintf(returnstr, "[PREVIOUS TRACK (Multimedia)]");
				break;
			case EXTENDED_STOP: 
				sprintf(returnstr, "[STOP (Multimedia)]");
				break;
			case EXTENDED_PLAYPAUSE:
				sprintf(returnstr, "[PLAY/PAUSE (Multimedia)]");
				break;
			case EXTENDED_MUTE:
				sprintf(returnstr, "[MUTE (Multimedia)]");
				break;
			case EXTENDED_VOLUMEUP:
				sprintf(returnstr, "[VOLUME UP (Multimedia)]");
				break;
			case EXTENDED_VOLUMEDOWN:
				sprintf(returnstr, "[VOLUME DOWN (Multimedia)]");
				break;
			case EXTENDED_MEDIASELECT:
				sprintf(returnstr, "[MEDIA SELECT (Multimedia)]");
				break;
			case EXTENDED_EMAIL:
				sprintf(returnstr, "[LAUNCH EMAIL (Multimedia)]");
				break;
			case EXTENDED_CALCULATOR:
				sprintf(returnstr, "[LAUNCH CALCULATOR (Multimedia)]");
				break;
			case EXTENDED_MYCOMPUTER:
				sprintf(returnstr, "[LAUNCH MY COMPUTER (Multimedia)]");
				break;
			case EXTENDED_WWWSEARCH:
				sprintf(returnstr, "[WWW SEARCH (Multimedia)]");
				break;
			case EXTENDED_WWWHOME:
				sprintf(returnstr, "[WWW HOME (Multimedia)]");
				break;
			case EXTENDED_WWWBACK:
				sprintf(returnstr, "[WWW BACK (Multimedia)]");
				break;
			case EXTENDED_WWWFORWARD:
				sprintf(returnstr, "[WWW FORWARD (Multimedia)]");
				break;
			case EXTENDED_WWWSTOP:
				sprintf(returnstr, "[WWW STOP (Multimedia)]");
				break;
			case EXTENDED_WWWREFRESH:
				sprintf(returnstr, "[WWW REFRESH (Multimedia)]");
				break;
			case EXTENDED_FAVORITES:
				sprintf(returnstr, "[WWW FAVORITES (Multimedia)]");
				break;
			default:
				sprintf(returnstr, "Unrecognized Key");
		}
	}
}

void PS2KeyboardAnalyzerResults::GetCommandName(char returnstr[], U64 code, double DeviceType)
{

	if(DeviceType==0)
	{
		switch(code)
		{

			case CMD_WRITE_LEDS:
				sprintf(returnstr, "SET LEDs");
				break;
			case CMD_ECHO:
					sprintf(returnstr, "SEND ECHO");
				break;
			case CMD_SET_SCANCODE_SET:
					sprintf(returnstr, "SET SCANCODE");
				break;
			case CMD_READ_ID:	
					sprintf(returnstr, "READ DEVICE ID");
				break;
			case CMD_SET_REPEAT:
					sprintf(returnstr, "SET REPEAT");
				break;
			case CMD_KEYBOARD_ENABLE: 
					sprintf(returnstr, "KEYBOARD ENABLE");
				break;
			case CMD_KEYBOARD_DISABLE:
					sprintf(returnstr, "KEYBOARD DISABLE");
				break;
			case CMD_SET_DEFAULTS:	
					sprintf(returnstr, "SET DEFAULTS");
				break;
			case CMD_SET_ALL_KEYS_REPEAT:
					sprintf(returnstr, "SET ALL KEYS REPEAT CODES");
				break;
			case CMD_SET_ALL_KEYS_MAKEBREAK_CODES:
					sprintf(returnstr, "SET ALL KEYS MAKE, BREAK CODES");
				break;
			case CMD_SET_ALL_KEYS_MAKE_CODES:
					sprintf(returnstr, "SET ALL KEYS MAKE CODES ONLY");
				break;
			case CMD_SET_ALL_REPEAT_AND_MAKEBREAK_CODES:
					sprintf(returnstr, "SET ALL KEYS MAKE, BREAK, REPEAT CODES");
				break;
			case CMD_SET_SINGLE_REPEAT:		
					sprintf(returnstr, "SET KEY REPEAT CODES");
				break;
			case CMD_SET_SINGLE_MAKEBREAK_CODES:
					sprintf(returnstr, "SET KEY MAKE, BREAK CODES");
				break;
			case CMD_SET_SINGLE_MAKE_CODES:	
					sprintf(returnstr, "SET KEY MAKE CODES ONLY");
				break;
			case CMD_RESEND:	
					sprintf(returnstr, "RESEND LAST DATA");
				break;
			case CMD_KEYBOARD_RESET:
					sprintf(returnstr, "RESET KEYBOARD");
				break;
			default:
				sprintf(returnstr, "DATA FRAME");
		}
	}
	else
	{	switch(code)
		{
			case MOUSE_CMD_SET_SCALING2to1:
				sprintf(returnstr, "SET SCALING TO 2:1");
				break;
			case MOUSE_CMD_SET_SCALING1to1:
				sprintf(returnstr, "SET SCALING 1:1");
				break;
			case MOUSE_CMD_SET_RESOLUTION:
				sprintf(returnstr, "SET RESOLUTION");
				break;
			case MOUSE_CMD_STATUS_REQUEST:
				sprintf(returnstr, "STATUS REQUEST");
				break;
			case MOUSE_CMD_SET_STREAM_MODE:
				sprintf(returnstr, "SET STREAM MODE");
				break;
			case MOUSE_CMD_READ_DATA:
				sprintf(returnstr, "READ DATA");
				break;
			case MOUSE_CMD_RESET_WRAP_MODE:	
				sprintf(returnstr, "RESET WRAP MODE");
				break;
			case MOUSE_CMD_SET_WRAP_MODE:
				sprintf(returnstr, "SET WRAP MODE");
				break;
			case MOUSE_CMD_SET_REMOTE_MODE:	
				sprintf(returnstr, "SET REMOTE MODE");
				break;
			case MOUSE_CMD_GET_DEVICE_ID:
				sprintf(returnstr, "GET DEVICE ID");
				break;
			case MOUSE_CMD_SET_SAMPLE_RATE:
				sprintf(returnstr, "SET SAMPLE RATE");
				break;
			case MOUSE_CMD_ENABLE_DATA_REPORTING:
				sprintf(returnstr, "ENABLE DATA REPORTING");
				break;
			case MOUSE_CMD_DISABLE_DATA_REPORTING:
				sprintf(returnstr, "DISABLE DATA REPORTING");
				break;
			case MOUSE_CMD_SET_DEFAULTS:
				sprintf(returnstr, "SET DEFAULTS");
				break;
			case MOUSE_CMD_RESEND:
				sprintf(returnstr, "RESEND");
				break;
			case MOUSE_CMD_RESET:
				sprintf(returnstr, "RESET MOUSE");
				break;
			default:
				sprintf(returnstr, "DATA FRAME");
		}
	}
}

void PS2KeyboardAnalyzerResults::GetKey(char returnstr[], U64 code, int keyset)
{
	if(keyset==0)
	{
		switch(code)
		{
			case KEY_ESC:
				sprintf(returnstr, "[ESC]");
				break;
			case KEY_F1:
				sprintf(returnstr, "[F1]");
				break;
			case KEY_F2:
				sprintf(returnstr, "[F2]");
				break;
			case KEY_F3:
				sprintf(returnstr, "[F3]");
				break;
			case KEY_F4:
				sprintf(returnstr, "[F4]");
				break;
			case KEY_F5:
				sprintf(returnstr, "[F5]");
				break;
			case KEY_F6:
				sprintf(returnstr, "[F6]");
				break;
			case KEY_F7:
				sprintf(returnstr, "[F7]");
				break;
			case KEY_F8:
				sprintf(returnstr, "[F8]");
				break;
			case KEY_F9:
				sprintf(returnstr, "[F9]");
				break;
			case KEY_F10:
				sprintf(returnstr, "[F10]");
				break;
			case KEY_F11:
				sprintf(returnstr, "[F11]");
				break;
			case KEY_F12:
				sprintf(returnstr, "[F12]");
				break;
			case KEY_SCROLLLOCK:
				sprintf(returnstr, "[SCROLL LOCK]");
				break;
			case KEY_ACCENT:
				sprintf(returnstr, "`");
				break;
			case KEY_MINUS:
				sprintf(returnstr, "-");
				break;
			case KEY_EQUAL:
				sprintf(returnstr, "=");
				break;
			case KEY_BACKSPACE:
				sprintf(returnstr, "[BACK SPACE]");
				break;
			case KEY_TAB:
				sprintf(returnstr, "\t");
				break;
			case KEY_LBRACE:
				sprintf(returnstr, "[");
				break;
			case KEY_RBRACE:
				sprintf(returnstr, "]");
				break;
			case KEY_BSLASH:
				sprintf(returnstr, "\\");
				break;
			case KEY_CAPSLOCK:
				sprintf(returnstr, "[CAPS]");
				break;
			case KEY_COLON:
				sprintf(returnstr, ";");
				break;
			case KEY_QUOTE:
				sprintf(returnstr, "'");
				break;
			case KEY_ENTER:
				sprintf(returnstr, "\n\r");
				break;
			case KEY_COMMA:
				sprintf(returnstr, ",");
				break;
			case KEY_PERIOD:
				sprintf(returnstr, ".");
				break;
			case KEY_FSLASH:
				sprintf(returnstr, "/");
				break;
			case KEY_LSHIFT:
				sprintf(returnstr, "[SHIFT (LEFT)]");
				break;
			case KEY_RSHIFT: 
				sprintf(returnstr, "[SHIFT (RIGHT)]");
				break;
			case KEY_LCONTROL: 
				sprintf(returnstr, "[CONTROL (LEFT)]");
				break;
			case KEY_LALT :
				sprintf(returnstr, "[ALT (LEFT)]");
				break;
			case KEY_SPACE :
				sprintf(returnstr, " ");
				break;
			case KEY_ASTERISK :
				sprintf(returnstr, "*");
				break;
			case KEY_NUMLOCK :
				sprintf(returnstr, "[NUM LOCK]");
				break;
			case KEY_NUMPAD_1 :
				sprintf(returnstr, "1");
				break;
			case KEY_NUMPAD_2 :
				sprintf(returnstr, "2");
				break;
			case KEY_NUMPAD_3 :
				sprintf(returnstr, "3");
				break;
			case KEY_NUMPAD_4 :
				sprintf(returnstr, "4");
				break;
			case KEY_NUMPAD_5 :
				sprintf(returnstr, "5");
				break;
			case KEY_NUMPAD_6 :
				sprintf(returnstr, "6");
				break;
			case KEY_NUMPAD_7 :
				sprintf(returnstr, "7");
				break;
			case KEY_NUMPAD_8 :
				sprintf(returnstr, "8");
				break;
			case KEY_NUMPAD_9 :
				sprintf(returnstr, "9");
				break;
			case KEY_NUMPAD_0 :
				sprintf(returnstr, "0");
				break;
			case KEY_NUMPAD_DECIMAL :
				sprintf(returnstr, ".");
				break;
			case KEY_NUMPAD_PLUS :
				sprintf(returnstr, "+");
				break;
			case KEY_NUMPAD_MINUS :
				sprintf(returnstr, "-");
				break;
			case KEY_Q :
				sprintf(returnstr, "Q");
				break;
			case KEY_W:
				sprintf(returnstr, "W");
				break;
			case KEY_E :
				sprintf(returnstr, "E");
				break;
			case KEY_R :
				sprintf(returnstr, "R");
				break;
			case KEY_T :
				sprintf(returnstr, "T");
				break;
			case KEY_Y :
				sprintf(returnstr, "Y");
				break;
			case KEY_U :
				sprintf(returnstr, "U");
				break;
			case KEY_I :
				sprintf(returnstr, "I");
				break;
			case KEY_O :
				sprintf(returnstr, "O");
				break;
			case KEY_P :
				sprintf(returnstr, "P");
				break;
			case KEY_A :
				sprintf(returnstr, "A");
				break; 
			case KEY_S :
				sprintf(returnstr, "S");
				break;
			case KEY_D :
				sprintf(returnstr, "D");
				break;
			case KEY_F :
				sprintf(returnstr, "F");
				break;
			case KEY_G :
				sprintf(returnstr, "G");
				break;
			case KEY_H :
				sprintf(returnstr, "H");
				break;
			case KEY_J :
				sprintf(returnstr, "J");
				break;
			case KEY_K:
				sprintf(returnstr, "K");
				break;
			case KEY_L :
				sprintf(returnstr, "L");
				break;
			case KEY_Z :
				sprintf(returnstr, "Z");
				break;
			case KEY_X :
				sprintf(returnstr, "X");
				break;
			case KEY_C :
				sprintf(returnstr, "C");
				break;
			case KEY_V :
				sprintf(returnstr, "V");
				break;
			case KEY_B :
				sprintf(returnstr, "B");
				break;
			case KEY_N :
				sprintf(returnstr, "N");
				break;
			case KEY_M :
				sprintf(returnstr, "M");
				break;
			case KEY_1 :
				sprintf(returnstr, "1");
				break;
			case KEY_2 :
				sprintf(returnstr, "2");
				break;
			case KEY_3 :
				sprintf(returnstr, "3");
				break;
			case KEY_4 :
				sprintf(returnstr, "4");
				break;
			case KEY_5 :
				sprintf(returnstr, "5");
				break;
			case KEY_6 :
				sprintf(returnstr, "6");
				break;
			case KEY_7 :
				sprintf(returnstr, "7");
				break;
			case KEY_8 :
				sprintf(returnstr, "8");
				break;
			case KEY_9 :
				sprintf(returnstr, "9");
				break;
			case KEY_0 :
				sprintf(returnstr, "0");
				break;
			default:
				sprintf(returnstr, "");
		}
	}
	else
	{
		switch(code)
		{
			case EXTENDED_RALT:
				sprintf(returnstr, "[ALT (RIGHT)]");
				break;
			case EXTENDED_RWINDOWS:
				sprintf(returnstr, "[WINDOWS (RIGHT)]");
				break;
			case EXTENDED_MENUS:
				sprintf(returnstr, "[MENU]");
				break;
			case EXTENDED_INSERT:
				sprintf(returnstr, "[INSERT]");
				break;
			case EXTENDED_HOME:
				sprintf(returnstr, "[HOME]");
				break;
			case EXTENDED_PAGEUP:
				sprintf(returnstr, "[PAGE UP]");
				break;
			case EXTENDED_DELETE:
				sprintf(returnstr, "[DELETE]");
				break;
			case EXTENDED_END:
				sprintf(returnstr, "[END]");
				break;
			case EXTENDED_PAGEDOWN:
				sprintf(returnstr, "[PAGE DOWN]");
				break;
			case EXTENDED_UPARROW:
				sprintf(returnstr, "[ARROW UP]");
				break;
			case EXTENDED_LEFTARROW:
				sprintf(returnstr, "[ARROW LEFT]");
				break;
			case EXTENDED_DOWNARROW:
				sprintf(returnstr, "[ARROW DOWN]");
				break;
			case EXTENDED_RIGHTARROW:
				sprintf(returnstr, "[ARROW RIGHT]");
				break;
			case EXTENDED_RCONTROL:
				sprintf(returnstr, "[CONTROL (RIGHT)]");
				break;
			case EXTENDED_LWINDOWS:
				sprintf(returnstr, "[WINDOWS (LEFT)]");
				break;
			case EXTENDED_NUMPAD_ENTER:
				sprintf(returnstr, "\n\r");
				break;
			case EXTENDED_FSLASH:
				sprintf(returnstr, "'\\'");
				break;
			case EXTENDED_POWER:
				sprintf(returnstr, "[POWER]");
				break;
			case EXTENDED_SLEEP:
				sprintf(returnstr, "[SLEEP]");
				break;
			case EXTENDED_WAKE:
				sprintf(returnstr, "[WAKE]");
				break;
			case EXTENDED_NEXTTRACK:
				sprintf(returnstr, "[NEXT TRACK (Multimedia)]");
				break;
			case EXTENDED_PREVIOUSTRACK:
				sprintf(returnstr, "[PREVIOUS TRACK (Multimedia)]");
				break;
			case EXTENDED_STOP: 
				sprintf(returnstr, "[STOP (Multimedia)]");
				break;
			case EXTENDED_PLAYPAUSE:
				sprintf(returnstr, "[PLAY/PAUSE (Multimedia)]");
				break;
			case EXTENDED_MUTE:
				sprintf(returnstr, "[MUTE (Multimedia)]");
				break;
			case EXTENDED_VOLUMEUP:
				sprintf(returnstr, "[VOLUME UP (Multimedia)]");
				break;
			case EXTENDED_VOLUMEDOWN:
				sprintf(returnstr, "[VOLUME DOWN (Multimedia)]");
				break;
			case EXTENDED_MEDIASELECT:
				sprintf(returnstr, "[MEDIA SELECT (Multimedia)]");
				break;
			case EXTENDED_EMAIL:
				sprintf(returnstr, "[LAUNCH EMAIL (Multimedia)]");
				break;
			case EXTENDED_CALCULATOR:
				sprintf(returnstr, "[LAUNCH CALCULATOR (Multimedia)]");
				break;
			case EXTENDED_MYCOMPUTER:
				sprintf(returnstr, "[LAUNCH MY COMPUTER (Multimedia)]");
				break;
			case EXTENDED_WWWSEARCH:
				sprintf(returnstr, "[WWW SEARCH (Multimedia)]");
				break;
			case EXTENDED_WWWHOME:
				sprintf(returnstr, "[WWW HOME (Multimedia)]");
				break;
			case EXTENDED_WWWBACK:
				sprintf(returnstr, "[WWW BACK (Multimedia)]");
				break;
			case EXTENDED_WWWFORWARD:
				sprintf(returnstr, "[WWW FORWARD (Multimedia)]");
				break;
			case EXTENDED_WWWSTOP:
				sprintf(returnstr, "[WWW STOP (Multimedia)]");
				break;
			case EXTENDED_WWWREFRESH:
				sprintf(returnstr, "[WWW REFRESH (Multimedia)]");
				break;
			case EXTENDED_FAVORITES:
				sprintf(returnstr, "[WWW FAVORITES (Multimedia)]");
				break;
			default:
				sprintf(returnstr, "");
		}
	}
}
