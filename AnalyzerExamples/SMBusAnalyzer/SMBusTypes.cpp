#include <cassert>

#include <algorithm>

#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>

#include "SMBusTypes.h"
#include "SMBusAnalyzer.h"
#include "SMBusCommands.h"

// protocols as defined by SMBus spec v2.0 chapter 5.5
const SMBusProtocol SMBusProtocols[NUM_SMBUS_PROTOCOLS] =
{
	{"Quick Command",	{SMBP_Start, SMBP_AddrAny, SMBP_Stop} },
	{"Send byte",		{SMBP_Start, SMBP_AddrWrite, SMBP_DataByte, SMBP_Stop} },
	{"Recv byte",		{SMBP_Start, SMBP_AddrRead, SMBP_DataByte, SMBP_Stop} },
	{"Write byte",		{SMBP_Start, SMBP_AddrWrite, SMBP_Command, SMBP_DataByte, SMBP_Stop} },
	{"Write word",		{SMBP_Start, SMBP_AddrWrite, SMBP_Command, SMBP_DataWord, SMBP_Stop} },
	{"Read byte",		{SMBP_Start, SMBP_AddrWrite, SMBP_Command, SMBP_Start, SMBP_AddrRead, SMBP_DataByte, SMBP_Stop} },
	{"Read word",		{SMBP_Start, SMBP_AddrWrite, SMBP_Command, SMBP_Start, SMBP_AddrRead, SMBP_DataWord, SMBP_Stop} },
	{"Block process call", {SMBP_Start, SMBP_AddrWrite, SMBP_Command, SMBP_DataWord, SMBP_Start, SMBP_AddrRead, SMBP_DataWord, SMBP_Stop} },
	{"Process call",	{SMBP_Start, SMBP_AddrWrite, SMBP_Command, SMBP_ByteCount, SMBP_DataBlock, SMBP_Stop} },
	{"Block write",		{SMBP_Start, SMBP_AddrWrite, SMBP_Command, SMBP_Start, SMBP_AddrRead, SMBP_ByteCount, SMBP_DataBlock, SMBP_Stop} },
	{"Block read",		{SMBP_Start, SMBP_AddrWrite, SMBP_Command, SMBP_ByteCount, SMBP_DataBlock, SMBP_Start, SMBP_AddrRead, SMBP_ByteCount, SMBP_DataBlock, SMBP_Stop} },
	{"Host notify",		{SMBP_Start, SMBP_AddrHostWrite, SMBP_DataByte, SMBP_DataByte, SMBP_Stop} },
};

#define SMBus_QuickCommand		(SMBusProtocols)
#define SMBus_SendByte			(SMBusProtocols + 1)
#define SMBus_RecvByte			(SMBusProtocols + 2)
#define SMBus_WriteByte			(SMBusProtocols + 3)
#define SMBus_WriteWord			(SMBusProtocols + 4)
#define SMBus_ReadByte			(SMBusProtocols + 5)
#define SMBus_ReadWord			(SMBusProtocols + 6)
#define SMBus_BlockProcessCall	(SMBusProtocols + 7)
#define SMBus_ProcessCall		(SMBusProtocols + 8)
#define SMBus_BlockWrite		(SMBusProtocols + 9)
#define SMBus_BlockRead			(SMBusProtocols + 10)
#define SMBus_HostNotify		(SMBusProtocols + 11)


// PMBus specific protocols as defined by PMBus spec v1.1 chapter 5.2
// PMBus Group command is handled separately
const SMBusProtocol PMBusProtocols[NUM_PMBUS_PROTOCOLS] =
{
	{"Ext command read byte",	{SMBP_Start, SMBP_AddrWrite, SMBP_CommandExt, SMBP_Start, SMBP_AddrRead, SMBP_DataByte, SMBP_Stop} },
	{"Ext command write byte",	{SMBP_Start, SMBP_AddrWrite, SMBP_CommandExt, SMBP_DataByte, SMBP_Stop} },
	{"Ext command read word",	{SMBP_Start, SMBP_AddrWrite, SMBP_CommandExt, SMBP_Start, SMBP_AddrRead, SMBP_DataByte, SMBP_DataByte, SMBP_Stop} },
	{"Ext command write word",	{SMBP_Start, SMBP_AddrWrite, SMBP_CommandExt, SMBP_DataByte, SMBP_DataByte, SMBP_Stop} },
};

// ************************************************************************************

Frame SMBusSignalState::ToFrame() const
{
	Frame f;

	f.mType = bus_signal;
	f.mStartingSampleInclusive = sample_begin;
	f.mEndingSampleInclusive = sample_end;
	f.mData1 = f.mData2 = 0;
	f.mFlags = 0;

	return f;
}

void SMBusSignalState::AddMarkers(AnalyzerResults* pResults, Channel& chnlSMBCLK, Channel& chnlSMBDAT)
{
	if (bus_signal == SMB_Zero  ||  bus_signal == SMB_One
			||  bus_signal == SMB_ACK  ||  bus_signal == SMB_NACK)
	{
		pResults->AddMarker(sample_rising_clk, AnalyzerResults::UpArrow, chnlSMBCLK);

		if (bus_signal == SMB_Zero)
			pResults->AddMarker(sample_marker, AnalyzerResults::Zero, chnlSMBDAT);
		else if (bus_signal == SMB_One)
			pResults->AddMarker(sample_marker, AnalyzerResults::One, chnlSMBDAT);
		else if (bus_signal == SMB_ACK)
			pResults->AddMarker(sample_marker, AnalyzerResults::Dot, chnlSMBDAT);
		else if (bus_signal == SMB_NACK)
			pResults->AddMarker(sample_marker, AnalyzerResults::ErrorDot, chnlSMBDAT);

	} else if (bus_signal == SMB_Start  ||  bus_signal == SMB_Stop) {
		pResults->AddMarker(sample_marker, bus_signal == SMB_Stop ? AnalyzerResults::Stop : AnalyzerResults::Start, chnlSMBDAT);
	}
}

// ************************************************************************************

SMBusByte::SMBusByte()
{
	Clear();
}

void SMBusByte::Clear()
{
	value = 0;
	is_acked = false;
	signals.clear();
}

Frame SMBusByte::ToFrame() const
{
	Frame f;

	f.mType = FT_Byte;
	f.mStartingSampleInclusive = signals.front().sample_begin;
	f.mEndingSampleInclusive = signals.back().sample_end;
	f.mData1 = value;
	f.mData2 = 0;
	f.mFlags = is_acked ? F_IsAcked : 0;

	return f;
}

Frame SMBusByte::ToAddrFrame() const
{
	Frame f;

	f.mType = FT_Address;
	f.mStartingSampleInclusive = signals.front().sample_begin;
	f.mEndingSampleInclusive = signals.back().sample_end;
	f.mData1 = value >> 1;
	f.mData2 = 0;
	f.mFlags = 0;
	if (is_acked)
		f.mFlags |= F_IsAcked;

	if (value & 1)
		f.mFlags |= F_IsRead;

	return f;
}

Frame SMBusByte::ToPMBusCommandFrame() const
{
	Frame f;

	f.mType = FT_CmdPMBus;
	f.mStartingSampleInclusive = signals.front().sample_begin;
	f.mEndingSampleInclusive = signals.back().sample_end;
	f.mData1 = value;
	f.mData2 = 0;
	f.mFlags = 0;
	if (is_acked)
		f.mFlags |= F_IsAcked;

	return f;
}

Frame SMBusByte::ToSMBusCommandFrame() const
{
	Frame f;

	f.mType = FT_CmdSMBus;
	f.mStartingSampleInclusive = signals.front().sample_begin;
	f.mEndingSampleInclusive = signals.back().sample_end;
	f.mData1 = value;
	f.mData2 = 0;
	f.mFlags = 0;
	if (is_acked)
		f.mFlags |= F_IsAcked;

	return f;
}

Frame SMBusByte::ToSmartBatteryCommandFrame() const
{
	Frame f;

	f.mType = FT_CmdSmartBattery;
	f.mStartingSampleInclusive = signals.front().sample_begin;
	f.mEndingSampleInclusive = signals.back().sample_end;
	f.mData1 = value;
	f.mData2 = 0;
	f.mFlags = 0;
	if (is_acked)		f.mFlags |= F_IsAcked;

	return f;
}

Frame SMBusByte::ToPMBusCommandExtFrame(const SMBusByte& sec) const
{
	Frame f;

	f.mType = FT_CmdPMBus;
	f.mStartingSampleInclusive = signals.front().sample_begin;
	f.mEndingSampleInclusive = sec.signals.back().sample_end;
	f.mData1 = value;
	f.mData1 <<= 8;
	f.mData1 |= sec.value;
	f.mData2 = 0;
	f.mFlags = 0;
	if (is_acked)
		f.mFlags |= F_IsAcked;

	return f;
}

Frame SMBusByte::ToDataWord(const SMBusByte& sec, SMBusFrameType frame_type) const
{
	Frame f;

	f.mType = frame_type;
	f.mStartingSampleInclusive = signals.front().sample_begin;
	f.mEndingSampleInclusive = sec.signals.back().sample_end;
	f.mData1 = value;
	f.mData1 <<= 8;
	f.mData1 |= sec.value;
	f.mData2 = 0;
	f.mFlags = 0;
	if (is_acked)
		f.mFlags |= F_IsAcked;

	return f;
}

Frame SMBusByte::ToByteCount() const
{
	Frame f;

	f.mType = FT_ByteCount;
	f.mStartingSampleInclusive = signals.front().sample_begin;
	f.mEndingSampleInclusive = signals.back().sample_end;
	f.mData1 = value;
	f.mData2 = 0;
	f.mFlags = 0;
	if (is_acked)
		f.mFlags |= F_IsAcked;

	return f;
}

Frame SMBusByte::ToDataByte(SMBusFrameType frame_type) const
{
	Frame f;

	f.mType = frame_type;
	f.mStartingSampleInclusive = signals.front().sample_begin;
	f.mEndingSampleInclusive = signals.back().sample_end;
	f.mData1 = value;
	f.mData2 = 0;
	f.mFlags = 0;
	if (is_acked)
		f.mFlags |= F_IsAcked;

	return f;
}

Frame SMBusByte::ToPECFrame(const U8 calcedPEC) const
{
	Frame f;

	f.mType = FT_PEC;
	f.mStartingSampleInclusive = signals.front().sample_begin;
	f.mEndingSampleInclusive = signals.back().sample_end;
	f.mData1 = value;
	f.mData2 = calcedPEC;
	f.mFlags = 0;
	if (is_acked)
		f.mFlags |= F_IsAcked;

	return f;
}

// ************************************************************************************

SMBusPacket::SMBusPacket()
{
	Clear();
}

void SMBusPacket::Clear()
{
	chunks.clear();
	first_start.Clear();
	stop.Clear();
}

void SMBusPacket::CreateDescFrames(SMBusAnalyzerResults* pResults, const SMBusProtocol* pProt, int desc_code) const
{
	// create the protocol description frames
	Frame clkf;

	clkf.mData1 = U64(pProt);
	clkf.mData2 = desc_code;
	clkf.mStartingSampleInclusive = chunks.front().front().signals.front().sample_begin;
	clkf.mEndingSampleInclusive = chunks.back().back().signals.back().sample_end;
	clkf.mType = FT_Desc;

	pResults->AddFrame(clkf);
}

void SMBusPacket::CreateFrames(SMBusAnalyzerResults* pResults, const SMBusProtocol* pProt, bool has_pec, SMBusDecodeLevel dec_level) const
{
	const SMBP_ProtElem* pProtElems = pProt->prot_elems;

	pResults->AddFrame(first_start.ToFrame());

	// first try if we have a PMBus command with non-trivial parameters
	if (dec_level == DL_PMBus  &&  pProt != SMBus_QuickCommand)
	{
		// first check the command code against the PMBus table
		const CommandDesc& cmd = GetPMBusCommandDesc(chunks.front()[1].value);

		// commands in Send byte SMBus protocol
		if (pProt == SMBus_SendByte
				&&  (cmd.id == CLEAR_FAULTS  ||  cmd.id == STORE_DEFAULT_ALL  ||  cmd.id == RESTORE_DEFAULT_ALL
						||  cmd.id == STORE_USER_ALL  ||  cmd.id == RESTORE_USER_ALL))
		{
			pResults->AddFrame(chunks.front()[0].ToAddrFrame());
			pResults->AddFrame(chunks.front()[1].ToPMBusCommandFrame());
			if (chunks.front().size() > 2)
				pResults->AddFrame(chunks.back().back().ToPECFrame(CalcPEC(has_pec)));

		} else if (pProt == SMBus_WriteByte
				&&  (cmd.direction == Write  ||  cmd.direction == ReadWrite)
				&&  cmd.databyte_param_type != FT_Undefined) {

			pResults->AddFrame(chunks.front()[0].ToAddrFrame());
			pResults->AddFrame(chunks.front()[1].ToPMBusCommandFrame());
			pResults->AddFrame(chunks.front()[2].ToDataByte(cmd.databyte_param_type));
			if (chunks.front().size() > 3)
				pResults->AddFrame(chunks.back().back().ToPECFrame(CalcPEC(has_pec)));

		} else if (pProt == SMBus_ReadByte
				&&  (cmd.direction == Read  ||  cmd.direction == ReadWrite)
				&&  cmd.databyte_param_type != FT_Undefined) {

			pResults->AddFrame(chunks.front()[0].ToAddrFrame());
			pResults->AddFrame(chunks.front()[1].ToPMBusCommandFrame());

			pResults->AddFrame(chunks[1][0].ToAddrFrame());
			pResults->AddFrame(chunks[1][1].ToDataByte(cmd.databyte_param_type));
			if (chunks.front().size() > 2)
				pResults->AddFrame(chunks.back().back().ToPECFrame(CalcPEC(has_pec)));

		} else if (pProt == SMBus_BlockProcessCall  &&  chunks.front()[1].value == QUERY
					&&  chunks.front().size() == 4 
					&&  ((has_pec  &&  chunks[1].size() == 4)  ||  (!has_pec  &&  chunks[1].size() == 3))) {

			// NOTE:
			// it is not possible (as far as I know) to distinguish between a ProcessCall
			// and a BlockProcessCall with byte counts of 1 - both have the same number of bytes

			pResults->AddFrame(chunks.front()[0].ToAddrFrame());
			pResults->AddFrame(chunks.front()[1].ToPMBusCommandFrame());
			pResults->AddFrame(chunks.front()[2].ToByteCount());
			pResults->AddFrame(chunks.front()[3].ToPMBusCommandFrame());

			pResults->AddFrame(chunks[1][0].ToAddrFrame());
			pResults->AddFrame(chunks[1][1].ToByteCount());
			pResults->AddFrame(chunks[1][2].ToDataByte(FT_PMBusQuery));
			if (chunks[1].size() > 3)
				pResults->AddFrame(chunks.back().back().ToPECFrame(CalcPEC(has_pec)));

		} else {
			CreateDefaultFrames(pResults, pProt, has_pec, dec_level);
		}

	} else if (dec_level == DL_SmartBattery  &&  pProt != SMBus_QuickCommand) {

		// get the command code
		const CommandDesc& cmd = GetSmartBatteryCommandDesc(chunks.front()[1].value);

		if (pProt == SMBus_WriteWord
				&&  (cmd.direction == Write  ||  cmd.direction == ReadWrite)
				&&  cmd.databyte_param_type != FT_Undefined)
		{
			pResults->AddFrame(chunks.front()[0].ToAddrFrame());
			pResults->AddFrame(chunks.front()[1].ToSmartBatteryCommandFrame());
			pResults->AddFrame(chunks.front()[2].ToDataWord(chunks.front()[3], cmd.databyte_param_type));
			if (chunks.front().size() > 4)
				pResults->AddFrame(chunks.back().back().ToPECFrame(CalcPEC(has_pec)));

		} else if (pProt == SMBus_ReadWord
				&&  (cmd.direction == Read  ||  cmd.direction == ReadWrite)
				&&  cmd.databyte_param_type != FT_Undefined) {

			pResults->AddFrame(chunks.front()[0].ToAddrFrame());
			pResults->AddFrame(chunks.front()[1].ToSmartBatteryCommandFrame());

			pResults->AddFrame(chunks[1][0].ToAddrFrame());
			pResults->AddFrame(chunks[1][1].ToDataWord(chunks[1][2], cmd.databyte_param_type));
			if (chunks[1].size() > 3)
				pResults->AddFrame(chunks.back().back().ToPECFrame(CalcPEC(has_pec)));
		} else {
			CreateDefaultFrames(pResults, pProt, has_pec, dec_level);
		}

	} else {
		CreateDefaultFrames(pResults, pProt, has_pec, dec_level);
	}

	pResults->AddFrame(stop.ToFrame());
}

void SMBusPacket::CreateDefaultFrames(SMBusAnalyzerResults* pResults, const SMBusProtocol* pProt, bool has_pec, SMBusDecodeLevel dec_level) const
{
	const SMBP_ProtElem* pProtElems = pProt->prot_elems;

	U8 prot_elem_ndx = 1;	// skip the initial Start

	size_t bcnt = 0, chnkcnt = 0;
	bool should_end_on_nack = false;
	int byte_count = -1;
	while (pProtElems[prot_elem_ndx] != SMBP_Stop)
	{
		const std::vector<SMBusByte>& chunk(chunks[chnkcnt]);

		switch (pProtElems[prot_elem_ndx])
		{
		case SMBP_Start:
			bcnt = 0;
			++chnkcnt;
			break;
		case SMBP_AddrAny:
		case SMBP_AddrWrite:
		case SMBP_AddrHostWrite:
			pResults->AddFrame(chunk[bcnt].ToAddrFrame());
			++bcnt;
			should_end_on_nack = false;
			break;
		case SMBP_AddrRead:
			pResults->AddFrame(chunk[bcnt].ToAddrFrame());
			++bcnt;
			should_end_on_nack = true;
			break;
		case SMBP_Command:
			if (dec_level == DL_PMBus)
				pResults->AddFrame(chunk[bcnt].ToPMBusCommandFrame());
			else if (dec_level == DL_SmartBattery)
				pResults->AddFrame(chunk[bcnt].ToSmartBatteryCommandFrame());
			else
				pResults->AddFrame(chunk[bcnt].ToSMBusCommandFrame());

			++bcnt;
			break;
		case SMBP_CommandExt:
			pResults->AddFrame(chunk[bcnt].ToPMBusCommandExtFrame(chunk[bcnt + 1]));
			bcnt += 2;
			break;
		case SMBP_DataByte:
			pResults->AddFrame(chunk[bcnt].ToDataByte());
			++bcnt;
			break;
		case SMBP_DataWord:
			pResults->AddFrame(chunk[bcnt].ToDataWord(chunk[bcnt + 1]));
			bcnt += 2;
			break;
		case SMBP_ByteCount:
			pResults->AddFrame(chunk[bcnt].ToByteCount());
			byte_count = chunk[bcnt].value;
			++bcnt;
			break;
		case SMBP_DataBlock:

			// create byte blocks
			int c;
			for (c = 0; c < byte_count; ++c)
				pResults->AddFrame(chunk[bcnt + c].ToDataByte());				

			bcnt += byte_count;

			byte_count = -1;

			break;
		}

		++prot_elem_ndx;
	}

	int bytes_remaining = chunks.back().size() - bcnt;

	if (has_pec  &&  bytes_remaining == 1)
		pResults->AddFrame(chunks.back().back().ToPECFrame(CalcPEC(has_pec)));
}

void SMBusPacket::CreateFramesForGroupCommand(SMBusAnalyzerResults* pResults, bool has_pec) const
{
	for (std::vector<std::vector<SMBusByte> >::const_iterator ci(chunks.begin());
				ci != chunks.end();
				++ci)
	{
		const std::vector<SMBusByte>& chunk(*ci);

		U8 pec = 0;

		pResults->AddFrame(chunk.front().ToAddrFrame());	// addr
		pec = SMBusCRCLookup[pec ^ chunk.front().value];
		pResults->AddFrame(chunk[1].ToPMBusCommandFrame());		// command
		pec = SMBusCRCLookup[pec ^ chunk[1].value];

		for (size_t bcnt = 2; bcnt < chunk.size() - (has_pec ? 1 : 0); ++bcnt)
		{
			pResults->AddFrame(chunk[bcnt].ToDataByte());
			pec = SMBusCRCLookup[pec ^ chunk[bcnt].value];
		}

		if (has_pec)
			pResults->AddFrame(chunk.back().ToPECFrame(pec));
	}
}

void SMBusPacket::CreateFramesForRawData(SMBusAnalyzerResults* pResults, bool has_pec) const
{
	for (std::vector<std::vector<SMBusByte> >::const_iterator ci(chunks.begin());
				ci != chunks.end();
				++ci)
	{
		const std::vector<SMBusByte>& chunk(*ci);

		U8 pec = 0;

		pResults->AddFrame(chunk.front().ToAddrFrame());	// addr
		pec = SMBusCRCLookup[pec ^ chunk.front().value];

		for (size_t bcnt = 1; bcnt < chunk.size() - (has_pec ? 1 : 0); ++bcnt)
		{
			pResults->AddFrame(chunk[bcnt].ToDataByte());
			pec = SMBusCRCLookup[pec ^ chunk[bcnt].value];
		}

		if (has_pec)
			pResults->AddFrame(chunk.back().ToPECFrame(pec));
	}
}

// CRC-8 lookup table for SMBus PEC calculation

const U8 SMBusCRCLookup[] =
{
0x00,0x07,0x0E,0x09,0x1C,0x1B,0x12,0x15,0x38,0x3F,0x36,0x31,0x24,0x23,0x2A,0x2D,
0x70,0x77,0x7E,0x79,0x6C,0x6B,0x62,0x65,0x48,0x4F,0x46,0x41,0x54,0x53,0x5A,0x5D,
0xE0,0xE7,0xEE,0xE9,0xFC,0xFB,0xF2,0xF5,0xD8,0xDF,0xD6,0xD1,0xC4,0xC3,0xCA,0xCD,
0x90,0x97,0x9E,0x99,0x8C,0x8B,0x82,0x85,0xA8,0xAF,0xA6,0xA1,0xB4,0xB3,0xBA,0xBD,
0xC7,0xC0,0xC9,0xCE,0xDB,0xDC,0xD5,0xD2,0xFF,0xF8,0xF1,0xF6,0xE3,0xE4,0xED,0xEA,
0xB7,0xB0,0xB9,0xBE,0xAB,0xAC,0xA5,0xA2,0x8F,0x88,0x81,0x86,0x93,0x94,0x9D,0x9A,
0x27,0x20,0x29,0x2E,0x3B,0x3C,0x35,0x32,0x1F,0x18,0x11,0x16,0x03,0x04,0x0D,0x0A,
0x57,0x50,0x59,0x5E,0x4B,0x4C,0x45,0x42,0x6F,0x68,0x61,0x66,0x73,0x74,0x7D,0x7A,
0x89,0x8E,0x87,0x80,0x95,0x92,0x9B,0x9C,0xB1,0xB6,0xBF,0xB8,0xAD,0xAA,0xA3,0xA4,
0xF9,0xFE,0xF7,0xF0,0xE5,0xE2,0xEB,0xEC,0xC1,0xC6,0xCF,0xC8,0xDD,0xDA,0xD3,0xD4,
0x69,0x6E,0x67,0x60,0x75,0x72,0x7B,0x7C,0x51,0x56,0x5F,0x58,0x4D,0x4A,0x43,0x44,
0x19,0x1E,0x17,0x10,0x05,0x02,0x0B,0x0C,0x21,0x26,0x2F,0x28,0x3D,0x3A,0x33,0x34,
0x4E,0x49,0x40,0x47,0x52,0x55,0x5C,0x5B,0x76,0x71,0x78,0x7F,0x6A,0x6D,0x64,0x63,
0x3E,0x39,0x30,0x37,0x22,0x25,0x2C,0x2B,0x06,0x01,0x08,0x0F,0x1A,0x1D,0x14,0x13,
0xAE,0xA9,0xA0,0xA7,0xB2,0xB5,0xBC,0xBB,0x96,0x91,0x98,0x9F,0x8A,0x8D,0x84,0x83,
0xDE,0xD9,0xD0,0xD7,0xC2,0xC5,0xCC,0xCB,0xE6,0xE1,0xE8,0xEF,0xFA,0xFD,0xF4,0xF3
};

U8 SMBusPacket::CalcPEC(bool has_pec) const
{
	U8 ret_val = 0;

	for (std::vector<std::vector<SMBusByte> >::const_iterator ci(chunks.begin());
					ci != chunks.end();
					++ci)
	{
		for (std::vector<SMBusByte>::const_iterator bi(ci->begin());
						bi != ci->end();
						++bi)
		{
			if (!has_pec  ||  ci != chunks.end() - 1  ||  bi != ci->end() - 1)
				ret_val = SMBusCRCLookup[ret_val ^ bi->value];
		}
	}

	return ret_val;
}

bool SMBusPacket::MatchesProtocol(const SMBusProtocol* pProt, bool has_pec) const
{
	U8 prot_elem_ndx = 1;	// skip the initial Start

	if (chunks.empty())
		return false;

	const SMBP_ProtElem* pProtElems = pProt->prot_elems;

	size_t bcnt = 0, chnkcnt = 0;
	int byte_count = -1;
	while (pProtElems[prot_elem_ndx] != SMBP_Stop)
	{
		if (chunks.size() <= chnkcnt)
			return false;

		const std::vector<SMBusByte>& chunk(chunks[chnkcnt]);

		if (chunk.empty())
			return false;

		if (pProtElems[prot_elem_ndx] != SMBP_Start  &&  pProtElems[prot_elem_ndx] != SMBP_Stop
				&&  chunk.size() <= bcnt)
			return false;

		switch (pProtElems[prot_elem_ndx])
		{
		case SMBP_Start:

			// check if we've consumed the previous chunks
			if (chunk.size() != bcnt)
				return false;

			bcnt = 0;
			++chnkcnt;
			break;
		case SMBP_AddrAny:
			++bcnt;
			break;
		case SMBP_AddrRead:
			if (!chunk[bcnt].IsRead())
				return false;

			++bcnt;
			break;
		case SMBP_AddrWrite:
			if (chunk[bcnt].IsRead())
				return false;

			++bcnt;
			break;
		case SMBP_AddrHostWrite:
			if (chunk[bcnt].GetAddr() != 0x04	// Host addr
					|| chunk[bcnt].IsRead())
				return false;

			++bcnt;
			break;
		case SMBP_Command:
			++bcnt;
			break;
		case SMBP_CommandExt:
			if (chunk[bcnt].value != PMBUS_COMMAND_EXT)
				return false;

			bcnt += 2;
			break;
		case SMBP_DataByte:
			++bcnt;
			break;
		case SMBP_DataWord:
			bcnt += 2;
			break;
		case SMBP_ByteCount:
			byte_count = chunk[bcnt].value;
			++bcnt;
			break;
		case SMBP_DataBlock:
			bcnt += byte_count;
			byte_count = -1;
			break;
		}

		++prot_elem_ndx;
	}

	if (chnkcnt != chunks.size() - 1)
		return false;

	int bytes_remaining = chunks.back().size() - bcnt;

	// if we have a quick command, it's OK for have 1 single byte packet without PEC
	if (chunks.size() == 1  &&  chunks.front().size() == 1  &&  bytes_remaining == 0)
		return true;

	if (has_pec)
		return bytes_remaining == 1;

	return bytes_remaining == 0;
}

bool SMBusPacket::MatchesGroupCommand(bool has_pec) const
{
	for (std::vector<std::vector<SMBusByte> >::const_iterator ci(chunks.begin());
				ci != chunks.end();
				++ci)
	{
		const std::vector<SMBusByte>& chunk(*ci);

		if (chunk.empty())
			return false;

		// we must have an addr write first
		if (chunk.front().value & 1)
			return false;

		// we must have at least one data byte + PEC (if present)
		if (chunk.size() < size_t(3 + (has_pec ? 1 : 0)))
			return false;
	}

	return true;
}

std::string int2str(const U8 i)
{
	char number_str[8];
	AnalyzerHelpers::GetNumberString(i, Decimal, 8, number_str, sizeof(number_str));
	return number_str;
}

std::string int2str_sal(const U64 i, DisplayBase base, const int max_bits)
{
	char number_str[256];
	AnalyzerHelpers::GetNumberString(i, base, max_bits, number_str, sizeof(number_str));
	return number_str;
}
