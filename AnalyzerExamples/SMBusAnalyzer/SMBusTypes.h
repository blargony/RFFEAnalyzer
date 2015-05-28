#ifndef SMBUS_TYPES_H
#define SMBUS_TYPES_H

#include <LogicPublicTypes.h>
#include <AnalyzerResults.h>

// the decoding level of the analyzer
enum SMBusDecodeLevel
{
	DL_Signals,
	DL_Bytes,
	DL_SMBus,
	DL_PMBus,
	DL_SmartBattery,
};

// SMBus states 
enum SMBusSignal
{
	SMB_Start,
	SMB_Stop,
	SMB_Zero,
	SMB_One,

	SMB_ACK,
	SMB_NACK,

	SMB_Undefined,
};

enum SMBusFrameType
{
	FT_Start,		// signals
	FT_Stop,
	FT_Zero,
	FT_One,
	FT_ACK,
	FT_NACK,

	FT_Byte,		// raw byte
	FT_Word,		// raw word

	FT_Address,		// address byte
	FT_PEC,

	FT_CmdSMBus,	// unknown SMBus command

	FT_CmdPMBus,	// commands
	FT_CmdSmartBattery,

	FT_ByteCount,	// the block count. it's followed by raw bytes

	FT_Desc,		// description frame that appears on the clock signal.
					// contains SMBus procotol packets names (ByteRead,...)


	// PMBus command params
	FT_PMBusCapability,
	FT_PMBusQuery,
	FT_PMBusWriteProtect,
	FT_PMBusOperation,
	FT_PMBusOnOffConfig,
	FT_PMBusVoutMode,

	// SmartBattery
	FT_SmartBattBatteryMode,
	FT_SmartBattBatteryStatus,
	FT_SmartBattSpecificationInfo,
	FT_SmartBattManufactureDate,

	FT_Undefined,
};

enum SMBusFlags
{
	F_IsAcked	= 0x01,
	F_IsRead	= 0x02,
};

struct SMBusSignalState
{
	U64		sample_begin;
	U64		sample_end;

	U64		sample_marker;

	U64		sample_rising_clk;	// for data bits

	SMBusSignal	bus_signal;

	SMBusSignalState()
	{
		Clear();
	}

	void Clear()
	{
		sample_begin = sample_end = sample_marker = sample_rising_clk = 0;

		bus_signal = SMB_Undefined;
	}

	Frame ToFrame() const;
	void AddMarkers(AnalyzerResults* pResults, Channel& chnlSMBCLK, Channel& chnlSMBDAT);

	bool IsEmpty() const
	{
		return sample_begin == sample_end  &&  sample_begin == 0;
	}
};

struct SMBusByte
{
	U8								value;
	bool							is_acked;
	std::vector<SMBusSignalState>	signals;

	SMBusByte();
	void Clear();

	bool IsComplete() const
	{
		return signals.size() == 9;		// 8 data + ack/nack
	}

	bool IsRead() const
	{
		return value & 1;
	}

	U8 GetAddr() const
	{
		return value >> 1;
	}

	Frame ToFrame() const;
	Frame ToAddrFrame() const;
	Frame ToSmartBatteryCommandFrame() const;
	Frame ToSMBusCommandFrame() const;
	Frame ToPMBusCommandFrame() const;
	Frame ToPMBusCommandExtFrame(const SMBusByte& sec) const;
	Frame ToDataByte(SMBusFrameType frame_type = FT_Byte) const;
	Frame ToDataWord(const SMBusByte& sec, SMBusFrameType frame_type = FT_Word) const;
	Frame ToPECFrame(const U8 calcedPEC) const;
	Frame ToByteCount() const;
};

// SMBus protocolelements that are parsable by small chunks of code
enum SMBP_ProtElem
{
	SMBP_Start,
	SMBP_Stop,
	SMBP_AddrAny,
	SMBP_AddrRead,
	SMBP_AddrWrite,
	SMBP_AddrHostWrite,
	SMBP_Command,
	SMBP_CommandExt,
	SMBP_DataByte,
	SMBP_DataWord,
	SMBP_ByteCount,
	SMBP_DataBlock,
};

struct SMBusProtocol
{
	const char*				name;				// the name of the protocol
	const SMBP_ProtElem		prot_elems[10];		// the protocol elements
};

#define NUM_SMBUS_PROTOCOLS		12
#define NUM_PMBUS_PROTOCOLS		4

extern const SMBusProtocol SMBusProtocols[NUM_SMBUS_PROTOCOLS];
extern const SMBusProtocol PMBusProtocols[NUM_PMBUS_PROTOCOLS];


class SMBusAnalyzerResults;

struct SMBusPacket
{
	SMBusSignalState		first_start;
	SMBusSignalState		stop;

	std::vector<std::vector<SMBusByte> >	chunks;

	SMBusPacket();
	void Clear();

	bool IsEmpty() const		{ return chunks.empty(); }

	// does not work for group command
	U8 CalcPEC(bool has_pec) const;

	// these return true if the packet matches the specified protocol
	bool MatchesProtocol(const SMBusProtocol* pProt, bool has_pec) const;
	bool MatchesGroupCommand(bool has_pec) const;

	void CreateFrames(SMBusAnalyzerResults* pResults, const SMBusProtocol* pProt, bool has_pec, SMBusDecodeLevel dec_level) const;
	void CreateDefaultFrames(SMBusAnalyzerResults* pResults, const SMBusProtocol* pProt, bool has_pec, SMBusDecodeLevel dec_level) const;
	void CreateDescFrames(SMBusAnalyzerResults* pResults, const SMBusProtocol* pProt, int desc_code) const;

	void CreateFramesForGroupCommand(SMBusAnalyzerResults* pResults, bool has_pec) const;
	void CreateFramesForRawData(SMBusAnalyzerResults* pResults, bool has_pec) const;
};

extern const U8 SMBusCRCLookup[];

enum ParamDirection
{
	ReadWrite,
	Read,
	Write,
	Send,
	Undefined	= -2,	// const also used for length
};

#define	LengthVariable		-1

// PEC calculation - CRC-8 with for a C(x) = x^8 + x^2 + x + 1 polynomial
U8 CalcPEC(const U8* pBytes, int numBytes);


// timings - not really used

#define ONE_MS		0.001
#define ONE_US		0.000001
#define ONE_NS		0.000000001

#define TBUF_MIN		(4.7 * ONE_US)		// Bus free time between Stop and Start Condition
#define THD_STA_MIN		(4 * ONE_US)		// Hold time after (Repeated) Start Condition.
											// After this period, the first clock is generated.
#define TSU_STA_MIN		(4.7 * ONE_US)		// Repeated Start Condition setup time
#define TSU_STO_MIN		(4 * ONE_US)		// Stop Condition setup time
#define THD_DAT_MIN		(300 * ONE_NS)		// Data hold time
#define TSU_DAT_MIN		(250 * ONE_NS)		// Data setup time
#define TTIMEOUT_MIN	(25 * ONE_MS)		// Detect clock low timeout
#define TTIMEOUT_MAX	(35 * ONE_MS)
#define TLOW_MIN		(4.7 * ONE_US)		// Clock low period
#define THIGH_MIN		(4 * ONE_US)		// Clock high period
#define THIGH_MAX		(50 * ONE_US)
#define TLOW_SEXT_MAX	(25 * ONE_MS)		// Cumulative clock low extend time (slave device)
#define TLOW_MEXT_MAX	(10 * ONE_MS)		// Cumulative clock low extend time (master device)
#define TF_MAX			(300 * ONE_NS)		// Clock/Data Fall Time
#define TR_MAX			(1000 * ONE_NS)		// Clock/Data Rise Time
#define TPOR_MAX		(500 * ONE_MS)		// Time in which a device must be operational after power-on reset

// helpers & debug
std::string int2str_sal(const U64 i, DisplayBase base, const int max_bits = 8);
inline std::string int2str(const U64 i)
{
	return int2str_sal(i, Decimal, 64);
}

/*
// debugging helper functions -- Windows only!!!
inline void debug(const std::string& str)
{
#if !defined(NDEBUG)  &&  defined(_WINDOWS)
	::OutputDebugStringA(("----- " + str + "\n").c_str());
#endif
}

inline void debug(const char* str)
{
#if !defined(NDEBUG)  &&  defined(_WINDOWS)
	debug(std::string(str));
#endif
}
*/

#endif	// SMBUS_TYPES_H
