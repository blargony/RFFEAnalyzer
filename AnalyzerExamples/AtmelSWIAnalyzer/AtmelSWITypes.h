#ifndef ATMEL_SWI_TYPES_H
#define ATMEL_SWI_TYPES_H

#include <LogicPublicTypes.h>
#include <AnalyzerResults.h>

// SWI timings in nano seconds, unless otherwise noted
enum SWI_Timings
{
	T_WLO_MIN			= 58000,		// this should be 60000 according to specs!!!
	T_WHI_MIN			= 2500000,

	/*
	T_HIGNORE_A			 45,
	T_LIGNORE_A			 45,
	T_HIGNORE_S		   1500,
	T_LIGNORE_S		   1500,

	T_WATCHDOG_ms	   1700,			// milli second
	*/

	T_DATA_PULSE_MIN	= 4100,
	T_DATA_PULSE_MAX	= 8600,

	T_ZHI_MIN			= 4100,
	T_ZHI_MAX			= 8600,

	T_ZLO_MIN			= 4100,
	T_ZLO_MAX			= 8600,

	T_BIT_MIN			= 35000,		// this should be 37000 according to specs!!!
	T_BIT_MAX			= 78000,

	T_TURNAROUND_MIN	= 28000,
	T_TURNAROUND_MAX	= 37000,
};

// SWI tokens
enum SWI_Token
{
	SWI_Wake,
	SWI_Zero,
	SWI_One,
};

class SWI_WaveParser
{
private:
	AnalyzerChannelData* mSDA;

	void ReadWave();
	void ScrollWave();

	// hold the wave durations
	enum {DUR_BUFF_SIZE = 4};
	U64 DurBuff[DUR_BUFF_SIZE];

	// the sample indexes for the beginning/ending of a token
	U64 SampleBegin;
	U64 SampleEnd;

	int BuffLastElem;

	// duration of one sample
	const double SampleDur_ns;

public:
	SWI_WaveParser(AnalyzerChannelData* pChannel, U64 sampleRateHz);

	// returns the next token from the stream
	SWI_Token GetToken(U64& smplBegin, U64& smplEnd);

	// reads tokens until it finds a wake token
	void GetWake(U64& smplBegin, U64& smplEnd);

	// returns the next byte from the stream, or returns with
	// isWake == true if a wake token is found in which case the return value is 0
	U8 GetByte(U64& smplBegin, U64& smplEnd, bool& isWake);
};

enum SWI_Flag
{
	SWIF_Command	= 0x77,
	SWIF_Transmit	= 0x88,
	SWIF_Idle		= 0xBB,
	SWIF_Sleep		= 0xCC,
};

enum SWI_Opcode
{
	SWIO_Undefined	= 0x00,

	SWIO_DeriveKey	= 0x1C,
	SWIO_DevRev		= 0x30,
	SWIO_GenDig		= 0x15,
	SWIO_HMAC		= 0x11,
	SWIO_CheckMac	= 0x28,
	SWIO_Lock		= 0x17,
	SWIO_MAC		= 0x08,
	SWIO_Nonce		= 0x16,
	SWIO_Pause		= 0x01,
	SWIO_Random		= 0x1B,
	SWIO_Read		= 0x02,
	SWIO_UpdateExtra	= 0x20,
	SWIO_Write		= 0x12,
};

enum SWI_ParamType
{
	PT_Undefined,		// used with status packet

	PT_Opcode,			// always 1 byte
	PT_RawBytes,		// variable length > 2
	PT_Status,			// always 1 byte
	PT_Zone,			// always 1 byte
	PT_Byte,			// always 1 byte
	PT_Word,			// always 2 bytes
	PT_DWord,			// always 4 bytes
};

struct SWI_PacketParam
{
	SWI_Opcode		ForOpcode;		// which command is this for
	bool			IsCommand;		// true if this belongs to a command param, false for response param

	const char*		Name;
	int				Length;
	int				ValidIfCount;
	SWI_ParamType	Type;
};

// The param data. This contains the definitions of all the params of all
// the SWI commands and their responses
extern SWI_PacketParam PacketParams[];

struct SWI_Block
{
	// the I/O block bytes starting with the Count and ending with the Checksum
	std::vector<U8>	Data;

	// these determine the type of the packet data
	bool		IsCommand;
	SWI_Opcode	Opcode;		// if IsCommand == true this will be a copy of Data[1],
							// otherwise it will contain the Opcode of the command that this block is a response to

	SWI_Block()
	{
		Clear();
	}

	void Clear()
	{
		Data.clear();
		IsCommand = false;
		Opcode = SWIO_Undefined;
	}

	U8 GetCount() const
	{
		return Data.front();
	}

	U16 GetChecksum() const
	{
		return (static_cast<U16>(Data.back()) << 8) | *(Data.end() - 2);
	}

	U16 CalcCRC() const;
	bool CheckCRC() const
	{
		return CalcCRC() == GetChecksum();
	}
};

const char* GetFlagName(SWI_Flag flag);

#endif	// ATMEL_SWI_TYPES_H