#include "PS2KeyboardSimulationDataGenerator.h"
#include "PS2KeyboardAnalyzerSettings.h"
#include "PS2KeyboardAnalyzerScanCodes.h"
#include <AnalyzerHelpers.h>

PS2KeyboardSimulationDataGenerator::PS2KeyboardSimulationDataGenerator()
{
}

PS2KeyboardSimulationDataGenerator::~PS2KeyboardSimulationDataGenerator()
{
}

void PS2KeyboardSimulationDataGenerator::Initialize( U32 simulation_sample_rate, PS2KeyboardAnalyzerSettings* settings )
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	//1 period = 6 micorseconds; half-period = 3 microseconds
	mClockGenerator.Init( 54000.00, simulation_sample_rate );
	mClock = mPS2KeyboardSimulationChannels.Add( mSettings->mClockChannel, mSimulationSampleRateHz, BIT_HIGH );
	mData = mPS2KeyboardSimulationChannels.Add( mSettings->mDataChannel, mSimulationSampleRateHz, BIT_HIGH );
}

U32 PS2KeyboardSimulationDataGenerator::GenerateSimulationData( U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample( largest_sample_requested, sample_rate, mSimulationSampleRateHz );

	while( mData->GetCurrentSampleNumber() < adjusted_largest_sample_requested )
	{
				
		if(mSettings->mDeviceType==0)
		{
				//simulate a keyboard
				//begin with device to host transmission
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
				
				//Standard Keys
				SendStandardKey(KEY_F1); 
				SendStandardKey(KEY_F2);
				SendStandardKey(KEY_F3); 
				SendStandardKey(KEY_F4); 
				SendStandardKey(KEY_F5); 
				SendStandardKey(KEY_F6); 
				SendStandardKey(KEY_F7); 
				SendStandardKey(KEY_F8); 
				SendStandardKey(KEY_F9); 
				SendStandardKey(KEY_F10); 
				SendStandardKey(KEY_F11); 
				SendStandardKey(KEY_F12); 

				SendStandardKey(KEY_ESC); 
				SendStandardKey(KEY_SCROLLLOCK);
				SendStandardKey(KEY_ACCENT);
				SendStandardKey(KEY_MINUS);
				SendStandardKey(KEY_EQUAL);
				SendStandardKey(KEY_BACKSPACE); 
				SendStandardKey(KEY_TAB);
				SendStandardKey(KEY_LBRACE);
				SendStandardKey(KEY_RBRACE); 
				SendStandardKey(KEY_BSLASH); 
				SendStandardKey(KEY_CAPSLOCK); 
				SendStandardKey(KEY_COLON); 
				SendStandardKey(KEY_QUOTE); 
				SendStandardKey(KEY_ENTER); 
				SendStandardKey(KEY_COMMA); 
				SendStandardKey(KEY_PERIOD); 
				SendStandardKey(KEY_FSLASH); 

				SendStandardKey(KEY_LSHIFT); 
				SendStandardKey(KEY_RSHIFT); 
				SendStandardKey(KEY_LCONTROL); 
				SendStandardKey(KEY_LALT); 
				SendStandardKey(KEY_SPACE); 
				SendStandardKey(KEY_ASTERISK); 
				SendStandardKey(KEY_NUMLOCK); 

				SendStandardKey(KEY_NUMPAD_1); 
				SendStandardKey(KEY_NUMPAD_2); 
				SendStandardKey(KEY_NUMPAD_3);
				SendStandardKey(KEY_NUMPAD_4); 
				SendStandardKey(KEY_NUMPAD_5); 
				SendStandardKey(KEY_NUMPAD_6); 
				SendStandardKey(KEY_NUMPAD_7); 
				SendStandardKey(KEY_NUMPAD_8); 
				SendStandardKey(KEY_NUMPAD_9); 
				SendStandardKey(KEY_NUMPAD_0); 
				SendStandardKey(KEY_NUMPAD_DECIMAL); 
				SendStandardKey(KEY_NUMPAD_PLUS); 
				SendStandardKey(KEY_NUMPAD_MINUS);

				SendStandardKey(KEY_Q); 
				SendStandardKey(KEY_W); 
				SendStandardKey(KEY_E); 
				SendStandardKey(KEY_R); 
				SendStandardKey(KEY_T); 
				SendStandardKey(KEY_Y); 
				SendStandardKey(KEY_U); 
				SendStandardKey(KEY_I); 
				SendStandardKey(KEY_O); 
				SendStandardKey(KEY_P);

				SendStandardKey(KEY_A); 
				SendStandardKey(KEY_S); 
				SendStandardKey(KEY_D); 
				SendStandardKey(KEY_F); 
				SendStandardKey(KEY_G); 
				SendStandardKey(KEY_H); 
				SendStandardKey(KEY_J); 
				SendStandardKey(KEY_K); 
				SendStandardKey(KEY_L); 

				SendStandardKey(KEY_Z); 
				SendStandardKey(KEY_X); 
				SendStandardKey(KEY_C); 
				SendStandardKey(KEY_V); 
				SendStandardKey(KEY_B); 
				SendStandardKey(KEY_N); 
				SendStandardKey(KEY_M); 

				SendStandardKey(KEY_1); 
				SendStandardKey(KEY_2); 
				SendStandardKey(KEY_3); 
				SendStandardKey(KEY_4); 
				SendStandardKey(KEY_5); 
				SendStandardKey(KEY_6); 
				SendStandardKey(KEY_7); 
				SendStandardKey(KEY_8); 
				SendStandardKey(KEY_9); 
				SendStandardKey(KEY_0); 
					
				//Extended Keys
				SendExtendedKey(EXTENDED_RALT);
				SendExtendedKey(EXTENDED_RWINDOWS);
				SendExtendedKey(EXTENDED_MENUS);
				SendExtendedKey(EXTENDED_INSERT);
				SendExtendedKey(EXTENDED_HOME);
				SendExtendedKey(EXTENDED_PAGEUP);
				SendExtendedKey(EXTENDED_DELETE);
				SendExtendedKey(EXTENDED_END);
				SendExtendedKey(EXTENDED_PAGEDOWN);
				SendExtendedKey(EXTENDED_UPARROW);
				SendExtendedKey(EXTENDED_LEFTARROW);
				SendExtendedKey(EXTENDED_DOWNARROW);
				SendExtendedKey(EXTENDED_RIGHTARROW);
				SendExtendedKey(EXTENDED_RCONTROL);
				SendExtendedKey(EXTENDED_LWINDOWS);
				SendExtendedKey(EXTENDED_NUMPAD_ENTER);
				SendExtendedKey(EXTENDED_FSLASH);
				SendExtendedKey(EXTENDED_POWER);
				SendExtendedKey(EXTENDED_SLEEP);
				SendExtendedKey(EXTENDED_WAKE);
				SendExtendedKey(EXTENDED_NEXTTRACK);
				SendExtendedKey(EXTENDED_PREVIOUSTRACK);
				SendExtendedKey(EXTENDED_STOP);
				SendExtendedKey(EXTENDED_PLAYPAUSE);
				SendExtendedKey(EXTENDED_MUTE);
				SendExtendedKey(EXTENDED_VOLUMEUP);
				SendExtendedKey(EXTENDED_VOLUMEDOWN);
				SendExtendedKey(EXTENDED_MEDIASELECT);
				SendExtendedKey(EXTENDED_EMAIL);
				SendExtendedKey(EXTENDED_CALCULATOR);
				SendExtendedKey(EXTENDED_MYCOMPUTER);
				SendExtendedKey(EXTENDED_WWWSEARCH);
				SendExtendedKey(EXTENDED_WWWHOME);
				SendExtendedKey(EXTENDED_WWWBACK);
				SendExtendedKey(EXTENDED_WWWFORWARD);
				SendExtendedKey(EXTENDED_WWWSTOP);
				SendExtendedKey(EXTENDED_WWWREFRESH);
				SendExtendedKey(EXTENDED_FAVORITES);

				//Pause/Break
				SendPauseBreakKey();

				//PrintScreen
				SendPrintScreenKey();

				//Now for the host->device transmissions
				HostSendByte(CMD_WRITE_LEDS);
				SendByte(0xFA);
				HostSendByte(0x07);
				SendByte(0xFA);

				HostSendByte(CMD_ECHO);
				SendByte(0xEE);

				HostSendByte(CMD_SET_SCANCODE_SET);
				SendByte(0xFA);
				HostSendByte(0x03);
				SendByte(0xFA);

				HostSendByte(CMD_SET_SCANCODE_SET);
				SendByte(0xFA);
				HostSendByte(0x02);
				SendByte(0xFA);

				HostSendByte(CMD_SET_SCANCODE_SET);
				SendByte(0xFA);
				HostSendByte(0x01);
				SendByte(0xFA);

				HostSendByte(CMD_SET_SCANCODE_SET);
				SendByte(0xFA);
				HostSendByte(0x00);
				SendByte(0x01);

				HostSendByte(CMD_READ_ID);
				SendByte(0xAB);
				SendByte(0x83);

				HostSendByte(CMD_SET_REPEAT);
				SendByte(0xFA);
				HostSendByte(0x00);
				SendByte(0xFA);

				HostSendByte(CMD_KEYBOARD_ENABLE);
				
				HostSendByte(CMD_KEYBOARD_DISABLE);
				
				HostSendByte(CMD_SET_DEFAULTS);
				SendByte(0xFA);

				HostSendByte(CMD_SET_ALL_KEYS_REPEAT);
				SendByte(0xFA);

				HostSendByte(CMD_SET_ALL_KEYS_MAKEBREAK_CODES);
				SendByte(0xFA);

				HostSendByte(CMD_SET_ALL_KEYS_MAKE_CODES);
				SendByte(0xFA);

				HostSendByte(CMD_SET_ALL_REPEAT_AND_MAKEBREAK_CODES);
				SendByte(0xFA);

				HostSendByte(CMD_SET_SINGLE_REPEAT);
				SendByte(0xFA);
				HostSendByte(KEY_Z);
				SendByte(0xFA);
				HostSendByte(CMD_ECHO);
				SendByte(0xEE);
				
				HostSendByte(CMD_SET_SINGLE_MAKEBREAK_CODES);
				SendByte(0xFA);
				HostSendByte(KEY_Z);
				SendByte(0xFA);
				HostSendByte(CMD_ECHO);
				SendByte(0xEE);
				
				HostSendByte(CMD_SET_SINGLE_MAKE_CODES);
				SendByte(0xFA);
				HostSendByte(KEY_Z);
				SendByte(0xFA);
				HostSendByte(CMD_ECHO);
				SendByte(0xEE);
				
				HostSendByte(CMD_RESEND);
				SendByte(0x05);
				
				HostSendByte(CMD_KEYBOARD_RESET);
				SendByte(0xFA);
				SendByte(0xAA);
			}
			else
			{
				//simulate a mouse

				SendMovementPacket(mSettings->mDeviceType);
				SendMovementPacket(mSettings->mDeviceType);
				SendMovementPacket(mSettings->mDeviceType);
				SendMovementPacket(mSettings->mDeviceType);
				SendMovementPacket(mSettings->mDeviceType);

				HostSendByte(MOUSE_CMD_SET_SCALING2to1);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_SET_SCALING1to1);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_SET_RESOLUTION);
				SendByte(0xFA);
				HostSendByte(0x03);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_STATUS_REQUEST);
				SendByte(0xFA);
				SendByte(0x77);
				SendByte(0x03);
				SendByte(0x64);

				HostSendByte(MOUSE_CMD_SET_STREAM_MODE);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_READ_DATA);
				SendByte(0xFA);
				SendMovementPacket(mSettings->mDeviceType);
			
				HostSendByte(MOUSE_CMD_RESET_WRAP_MODE);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_SET_WRAP_MODE);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_SET_REMOTE_MODE);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_GET_DEVICE_ID);
				SendByte(0xFA);
				SendMouseDeviceID(mSettings->mDeviceType);

				HostSendByte(MOUSE_CMD_SET_SAMPLE_RATE);
				SendByte(0xFA);
				HostSendByte(0x64);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_ENABLE_DATA_REPORTING);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_DISABLE_DATA_REPORTING);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_SET_DEFAULTS);
				SendByte(0xFA);

				HostSendByte(MOUSE_CMD_RESEND);
				SendByte(0xFA);
				SendMovementPacket(mSettings->mDeviceType);

				HostSendByte(MOUSE_CMD_RESET);
				SendByte(0xFA);
				SendByte(0xAA);
				SendMouseDeviceID(mSettings->mDeviceType);

			}

	}

	*simulation_channels = mPS2KeyboardSimulationChannels.GetArray();
	return mPS2KeyboardSimulationChannels.GetCount();
}

void PS2KeyboardSimulationDataGenerator::SendByte(U8 payload)
{
	
			mClock->TransitionIfNeeded(BIT_HIGH);
			mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));
	

			//make a copy of the data for calculating parity bit later
			U8 copyofpayload = payload;

			//start bit
			mData->TransitionIfNeeded(BIT_LOW);
			
			mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));

			mClock->Transition();
			mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));
			
			mClock->Transition();
			mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
		
			//data, lsb first
			for(int i=0;i<8;i++)
			{
				if(payload & 0x01)
				{
					mData->TransitionIfNeeded(BIT_HIGH);
					payload = payload >> 1;
				}
				else
				{
					mData->TransitionIfNeeded(BIT_LOW);
					payload = payload >> 1;
				}
				
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));

				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));
			
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
			
			}
			//parity bit
				int ones = 0;

				//sum the ones
				for(U32 j=0;j<8;j++)
				{
					ones=ones+(copyofpayload&0x01);
					copyofpayload=copyofpayload>>1;
				}

				//if lsb of ones is 1, then there is an odd number of ones, therefore parity =0
				
				if(ones&0x01)
					mData->TransitionIfNeeded(BIT_LOW);
				else
					mData->TransitionIfNeeded(BIT_HIGH);

				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));
			
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));

			//stop bit
				mData->TransitionIfNeeded(BIT_HIGH);
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));
				
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));
			
			//advance to the next time of transmission
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(.001));

}

void PS2KeyboardSimulationDataGenerator::SendStandardKey(U8 keycode)
{
	//Standard Make Code
	SendByte(keycode);
			
	//Standard Break Code
	SendByte(0xF0);
	SendByte(keycode);
}
void PS2KeyboardSimulationDataGenerator::SendExtendedKey(U8 keycode)
{
	//Make
	SendByte(0xE0);
	SendByte(keycode);

	//break		
	SendByte(0xE0);
	SendByte(0xF0);
	SendByte(keycode);
}
void PS2KeyboardSimulationDataGenerator::SendPrintScreenKey()
{
	//make
	SendByte(0xE0);
	SendByte(0x12);
	SendByte(0xE0);
	SendByte(0x7C);

	//break
	SendByte(0xE0);
	SendByte(0xF0);
	SendByte(0x7C);
	SendByte(0xE0);
	SendByte(0xF0);
	SendByte(0x12);
}
void PS2KeyboardSimulationDataGenerator::SendPauseBreakKey()
{
	//make
	SendByte(0xE1);
	SendByte(0x14);
	SendByte(0x77);
	SendByte(0xE1);
	SendByte(0xF0);
	SendByte(0x14);
	SendByte(0xF0);
	SendByte(0x77);

	//there is no break code for this key
}

void PS2KeyboardSimulationDataGenerator::SendMovementPacket(double MouseType)
{
	
	if(MouseType==1)
	{
		//standard ps/2 mouse sends just 3 bytes of info
		SendByte(0x08);
		SendByte(0x00);
		SendByte(0x00);
	}
	else
	{
		//IntelliMouse sends 4 bytes of info
		SendByte(0x08);
		SendByte(0x00);
		SendByte(0x00);
		SendByte(0x00);
	}

}

void PS2KeyboardSimulationDataGenerator::SendMouseDeviceID(double MouseType)
{
	if(MouseType==1)
		SendByte(0x00);
	else
		SendByte(0x03);

}

void PS2KeyboardSimulationDataGenerator::HostSendByte(U8 payload)
{
			//make a copy of the data for calculating parity bit later
			U8 copyofpayload = payload;

			//start bit
			mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
			
			mClock->TransitionIfNeeded(BIT_HIGH);
			mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));

			//bring the clock low for at least 100 microseconds
			mClock->TransitionIfNeeded(BIT_LOW);
			mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
			mData->TransitionIfNeeded(BIT_LOW);
			mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(.000125));
		
			//data, lsb first
			for(int i=0;i<8;i++)
			{
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));

				if(payload & 0x01)
				{
					mData->TransitionIfNeeded(BIT_HIGH);
					payload = payload >> 1;
				}
				else
				{
					mData->TransitionIfNeeded(BIT_LOW);
					payload = payload >> 1;
				}
				
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
			
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));
			
			}
			//parity bit
				int ones = 0;

				//sum the ones
				for(U32 j=0;j<8;j++)
				{
					ones=ones+(copyofpayload&0x01);
					copyofpayload=copyofpayload>>1;
				}

				//if lsb of ones is 1, then there is an odd number of ones, therefore parity =0
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));

				if(ones&0x01)
					mData->TransitionIfNeeded(BIT_LOW);
				else
					mData->TransitionIfNeeded(BIT_HIGH);

				
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
			
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));
			
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));


			//stop bit
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
				
				mData->TransitionIfNeeded(BIT_HIGH);
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));
				
				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));

			//simulate the ACK
				mData->TransitionIfNeeded(BIT_LOW);
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));

				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));

				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(1));

			//data line idle state is BIT_HIGH
				mData->TransitionIfNeeded(BIT_HIGH);

				mClock->Transition();
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));
				mClock->Transition();

			//advance to the next time of transmission
				mPS2KeyboardSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByTimeS(.001));

}
