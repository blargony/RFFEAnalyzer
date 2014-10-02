#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>

#include <algorithm>

#include "AtmelSWITypes.h"

// This table contains parameter data descriptions.
SWI_PacketParam PacketParams[] =
{
	// Status/error response -- this must be the first entry in the table
	{SWIO_Undefined,	false,	"Status",		1,	0, PT_Status},

	// CheckMac
	{SWIO_CheckMac,		true,	"CheckMac",		1,	0, PT_Opcode},
	{SWIO_CheckMac,		true,	"Mode",			1,	0, PT_Byte},
	{SWIO_CheckMac,		true,	"KeyID",		2,	0, PT_Word},
	{SWIO_CheckMac,		true,	"ClientChal",	32,	0, PT_RawBytes},
	{SWIO_CheckMac,		true,	"ClientResp",	32,	0, PT_RawBytes},
	{SWIO_CheckMac,		true,	"OtherData",	13,	0, PT_RawBytes},

	// DeriveKey
	{SWIO_DeriveKey,	true,	"DeriveKey",	1,	0, PT_Opcode},
	{SWIO_DeriveKey,	true,	"Random",		1,	0, PT_Byte},
	{SWIO_DeriveKey,	true,	"TargetKey",	2,	0, PT_Word},
	{SWIO_DeriveKey,	true,	"Mac",			32,	39, PT_RawBytes},

	// DevRev
	{SWIO_DevRev,		true,	"DevRev",		1,	0, PT_Opcode},
	{SWIO_DevRev,		true,	"Zero",			1,	0, PT_Byte},
	{SWIO_DevRev,		true,	"Zero",			2,	0, PT_Word},

	{SWIO_DevRev,		false,	"RevNum",		4,	0, PT_DWord},

	// GenDig
	{SWIO_GenDig,		true,	"GenDig",		1,	0, PT_Opcode},
	{SWIO_GenDig,		true,	"Zone",			1,	0, PT_Byte},
	{SWIO_GenDig,		true,	"KeyID",		2,	0, PT_Word},
	{SWIO_GenDig,		true,	"OtherData",	4,	11, PT_DWord},

	// HMAC
	{SWIO_HMAC,			true,	"HMAC",			1,	0, PT_Opcode},
	{SWIO_HMAC,			true,	"Mode",			1,	0, PT_Byte},
	{SWIO_HMAC,			true,	"KeyID",		2,	0, PT_Word},

	{SWIO_HMAC,			false,	"Response",		32,	0, PT_RawBytes},

	// Lock
	{SWIO_Lock,			true,	"Lock",			1,	0, PT_Opcode},
	{SWIO_Lock,			true,	"Zone",			1,	0, PT_Byte},
	{SWIO_Lock,			true,	"Summary",		2,	0, PT_Word},

	// MAC
	{SWIO_MAC,			true,	"MAC",			1,	0, PT_Opcode},
	{SWIO_MAC,			true,	"Mode",			1,	0, PT_Byte},
	{SWIO_MAC,			true,	"KeyID",		2,	0, PT_Word},

	{SWIO_MAC,			false,	"Response",		32,	0, PT_RawBytes},

	// Nonce
	{SWIO_Nonce,		true,	"Nonce",		1,	0, PT_Opcode},
	{SWIO_Nonce,		true,	"Mode",			1,	0, PT_Byte},
	{SWIO_Nonce,		true,	"Zero",			2,	0, PT_Word},
	{SWIO_Nonce,		true,	"NumIn",		20,	27, PT_RawBytes},
	{SWIO_Nonce,		true,	"NumIn",		32,	39, PT_RawBytes},

	{SWIO_Nonce,		false,	"RandOut",		32,	0, PT_RawBytes},

	// Pause
	{SWIO_Pause,		true,	"Pause",		1,	0, PT_Opcode},
	{SWIO_Pause,		true,	"Selector",		1,	0, PT_Byte},
	{SWIO_Pause,		true,	"Zero",			2,	0, PT_Word},

	// Random
	{SWIO_Random,		true,	"Random",		1,	0, PT_Opcode},
	{SWIO_Random,		true,	"Mode",			1,	0, PT_Byte},
	{SWIO_Random,		true,	"Zero",			2,	0, PT_Word},

	{SWIO_Random,		false,	"RandOut",		32,	0, PT_RawBytes},

	// Read
	{SWIO_Read,			true,	"Read",			1,	0, PT_Opcode},
	{SWIO_Read,			true,	"Zone",			1,	0, PT_Zone},
	{SWIO_Read,			true,	"Address",		2,	0, PT_Word},

	{SWIO_Read,			false,	"Contents",		32,	35, PT_RawBytes},
	{SWIO_Read,			false,	"Contents",		4,	7, PT_DWord},

	// UpdateExtra
	{SWIO_UpdateExtra,	true,	"UpdateExtra",	1,	0, PT_Opcode},
	{SWIO_UpdateExtra,	true,	"Mode",			1,	0, PT_Byte},
	{SWIO_UpdateExtra,	true,	"NewValue",		2,	0, PT_Word},

	// Write
	{SWIO_Write,		true,	"Write",		1,	0, PT_Opcode},
	{SWIO_Write,		true,	"Zone",			1,	0, PT_Zone},
	{SWIO_Write,		true,	"Address",		2,	0, PT_Word},
	{SWIO_Write,		true,	"Value",		4,	11, PT_DWord},
	{SWIO_Write,		true,	"Value",		4,	43, PT_DWord},
	{SWIO_Write,		true,	"Value",		32,	39, PT_RawBytes},
	{SWIO_Write,		true,	"Value",		32,	71, PT_RawBytes},
	{SWIO_Write,		true,	"Mac",			32,	43, PT_RawBytes},
	{SWIO_Write,		true,	"Mac",			32,	71, PT_RawBytes},

	// Table terminator
	{SWIO_Undefined,	true,	NULL,			0,	0, PT_Undefined},
};

SWI_WaveParser::SWI_WaveParser(AnalyzerChannelData* pChannel, U64 sampleRateHz)
:	SampleDur_ns(1e9 / sampleRateHz),
	mSDA(pChannel),
	BuffLastElem(0)
{}

void SWI_WaveParser::ReadWave()
{
	// get the low duration
	DurBuff[BuffLastElem++] = static_cast<U64>((mSDA->GetSampleOfNextEdge() - mSDA->GetSampleNumber()) * SampleDur_ns);
	mSDA->AdvanceToNextEdge();

	// get the high duration
	// we need the duration of the last high state, that's why we can't let GetSampleOfNextEdge()
	// cause the thread to exit. So we have to test if there are more transitions
	if (mSDA->DoMoreTransitionsExistInCurrentData())
	{
		DurBuff[BuffLastElem++] = static_cast<U64>((mSDA->GetSampleOfNextEdge() - mSDA->GetSampleNumber()) * SampleDur_ns);
		mSDA->AdvanceToNextEdge();
	} else {
		DurBuff[BuffLastElem++] = (T_BIT_MAX + T_BIT_MIN) / 2;
		mSDA->AdvanceToAbsPosition(static_cast<U64>(mSDA->GetSampleNumber() + ((T_BIT_MAX + T_BIT_MIN) / 2) / SampleDur_ns));
	}
}

SWI_Token SWI_WaveParser::GetToken(U64& smplBegin, U64& smplEnd)
{
	SWI_Token ret_val;

	// start from the high state
	if (mSDA->GetBitState() == BIT_HIGH)
		mSDA->AdvanceToNextEdge();

	for (;;)
	{
		// read a wave period
		if (BuffLastElem == 0)
		{
			smplBegin = mSDA->GetSampleNumber();
			ReadWave();
		}

		// is this a wake token?
		if (DurBuff[0] >= T_WLO_MIN  &&  DurBuff[1] >= T_WHI_MIN)
		{
			ret_val = SWI_Wake;
			break;
		}

		// is this a logic 1?
		if (DurBuff[0] >= T_DATA_PULSE_MIN  &&  DurBuff[0] <= T_DATA_PULSE_MAX
					&&  DurBuff[0] + DurBuff[1] >= T_BIT_MIN  /*&&  DurBuff[0] + DurBuff[1] <= T_BIT_MAX*/)
		{
			ret_val = SWI_One;
			break;
		}

		// get another low/high pair duration in case we have a logic zero
		ReadWave();

		// is this a logic 0?
		U64 bit_sum_dur = DurBuff[3];
		U8 wave_cnt;
		bool bit_ok = true;
		for (wave_cnt = 0; wave_cnt < 3  &&  bit_ok; ++wave_cnt)
		{
			bit_sum_dur += DurBuff[wave_cnt];
			bit_ok = (DurBuff[wave_cnt] >= T_DATA_PULSE_MIN  &&  DurBuff[wave_cnt] <= T_DATA_PULSE_MAX);
		}

		// is this a logic 0?
		if (bit_ok  &&  bit_sum_dur >= T_BIT_MIN /* &&  bit_sum_dur <= T_BIT_MAX*/)
		{
			ret_val = SWI_Zero;
			break;
		}

		// scroll the two last elements to the beginning
		DurBuff[0] = DurBuff[2];
		DurBuff[1] = DurBuff[3];
		BuffLastElem -= 2;
		smplBegin = mSDA->GetSampleNumber();
	}

	// set the ending sample
	smplEnd = mSDA->GetSampleNumber();

	// shorten off the unnecessary long token duration
	if (ret_val == SWI_Wake)
	{
		if ((smplEnd - smplBegin) * SampleDur_ns > T_WLO_MIN + T_WHI_MIN)
			smplEnd = static_cast<U64>(smplBegin + (T_WLO_MIN + T_WHI_MIN) / SampleDur_ns);
	} else {
		if ((smplEnd - smplBegin) * SampleDur_ns > T_BIT_MAX)
			smplEnd = static_cast<U64>(smplBegin + T_BIT_MAX / SampleDur_ns);
	}

	// consume the elements in the buffer
	BuffLastElem = 0;

	return ret_val;
}

void SWI_WaveParser::GetWake(U64& smplBegin, U64& smplEnd)
{
	while (GetToken(smplBegin, smplEnd) != SWI_Wake)
		;
}

U8 SWI_WaveParser::GetByte(U64& smplBegin, U64& smplEnd, bool& isWake)
{
	U8 ret_val = 0;
	int b;
	SWI_Token t;
	U64 locBegin;

	isWake = false;

	for (b = 0; b < 8; b++)
	{
		ret_val >>= 1;
		
		t = GetToken(locBegin, smplEnd);
		
		if (t == SWI_One)
			ret_val |= (1 << 7);
		else if (t == SWI_Wake) {
			smplBegin = locBegin;
			isWake = true;
			return 0;
		}

		if (b == 0)
			smplBegin = locBegin;
	}

	return ret_val;
}

const char* GetFlagName(SWI_Flag flag)
{
	switch (flag)
	{
	case SWIF_Transmit:
		return "Transmit";
	case SWIF_Command:
		return "Command";
	case SWIF_Idle:
		return "Idle";
	case SWIF_Sleep:
		return "Sleep";
	};

	return "<unknown>";
}

U16 SWI_Block::CalcCRC() const
{
	size_t length = Data.size() - 2;
	size_t counter;
	U16 crc_register = 0;
	U16 polynom = 0x8005;
	U8 shift_register, data_bit, crc_bit;

	for (counter = 0; counter < length; counter++)
	{
		for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1)
		{
			data_bit = (Data[counter] & shift_register) ? 1 : 0;
			crc_bit = crc_register >> 15;

			crc_register <<= 1;

			if ((data_bit ^ crc_bit) != 0)
				crc_register ^= polynom;
		}
	}

	return crc_register;
}
