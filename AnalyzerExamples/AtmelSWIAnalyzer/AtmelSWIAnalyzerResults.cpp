#include <iostream>
#include <fstream>
#include <algorithm>

#include <AnalyzerHelpers.h>

#include "AtmelSWIAnalyzerResults.h"
#include "AtmelSWIAnalyzer.h"
#include "AtmelSWIAnalyzerSettings.h"

// groups the output into 64bit segments
std::string GetByteArrayNumberStr(std::vector<U8>::const_iterator i, int num_bytes, DisplayBase display_base)
{
	const int BUFF_SIZE = 128;
	char number_str[BUFF_SIZE];

	std::string ret_val;
	int cnt_byte, cnt_u64;
	int bytes_remaining = num_bytes;

	for (cnt_u64 = 0; cnt_u64 < 4  &&  bytes_remaining > 0; ++cnt_u64, bytes_remaining -= 8)
	{
		if (!ret_val.empty())
			ret_val += '-';

		U64 val = 0;

		// byte arrays appear on the bus least significant byte first
		for (cnt_byte = 0; cnt_byte < (bytes_remaining > 8 ? 8 : bytes_remaining); ++cnt_byte, ++i)
		{
			val <<= 8;
			val |= *i;
		}

		// make the string
		AnalyzerHelpers::GetNumberString(val, display_base, 
					bytes_remaining > 8 ? 64 : bytes_remaining * 8, number_str, BUFF_SIZE);

		ret_val += number_str;
	}

	return ret_val;
}

AtmelSWIAnalyzerResults::AtmelSWIAnalyzerResults(AtmelSWIAnalyzer* analyzer, AtmelSWIAnalyzerSettings* settings)
:	mSettings(settings),
	mAnalyzer(analyzer)
{}

AtmelSWIAnalyzerResults::~AtmelSWIAnalyzerResults()
{}

void AtmelSWIAnalyzerResults::GetTextsForPacketSegmentFrame(const Frame& f, DisplayBase display_base, std::vector<std::string>& texts)
{
	const int BUFF_SIZE = 128;
	char number_str[BUFF_SIZE];
	std::string tmpstr;

	texts.clear();

	// get the I/O block
	int block_ndx(static_cast<int>(f.mData2 & 0xffffffff));
	int block_offset(static_cast<int>(((f.mData2 >> 32) & 0xffffffff)));

	const SWI_Block& block(mBlocks[block_ndx]);

	SWI_PacketParam* param(PacketParams + f.mData1);

	switch (param->Type)
	{
	case PT_Opcode:

		AnalyzerHelpers::GetNumberString(block.Opcode, display_base, 8, number_str, BUFF_SIZE);

		texts.push_back(std::string("Opcode ") + param->Name + " (" + number_str + ")");

		texts.push_back(number_str);
		texts.push_back(param->Name);
		texts.push_back(std::string("Opcode ") + param->Name);
		texts.push_back(std::string(param->Name) + " (" + number_str + ")");

		break;
	case PT_RawBytes:
		{
		std::string array_str(GetByteArrayNumberStr(block.Data.begin() + block_offset, param->Length, display_base));

		texts.push_back(std::string(param->Name) + " (" + array_str + ")");

		texts.push_back(array_str);
		texts.push_back(param->Name);
		}

		break;
	case PT_Zone:
		{
		AnalyzerHelpers::GetNumberString(block.Opcode, display_base, 8, number_str, BUFF_SIZE);

		U8 zone = block.Data[block_offset] & 3;
		std::string zonestr("<unknown>");
		if (zone == 0)
			zonestr = "Config";
		else if (zone == 1)
			zonestr = "OTP";
		else if (zone == 2)
			zonestr = "Data";

		const char* lenstr;
		if (block.Data[block_offset] & 0x80)
			lenstr = "32 bytes";
		else
			lenstr = "4 bytes";

		texts.push_back("Zone " + zonestr + ", " + lenstr);

		texts.push_back(number_str);
		texts.push_back(std::string(param->Name) + " (" + number_str + ")");
		}
		break;
	case PT_Byte:
		AnalyzerHelpers::GetNumberString(block.Data[block_offset], display_base, 8, number_str, BUFF_SIZE);
		
		texts.push_back(std::string(param->Name) + " (" + number_str + ")");
		texts.push_back(param->Name);

		break;

	case PT_Status:
		AnalyzerHelpers::GetNumberString(block.Data[block_offset], display_base, 8, number_str, BUFF_SIZE);

		tmpstr = "<undefined>";
		if (block.Data[block_offset] == 0x00)
			tmpstr = "Command executed successfully";
		else if (block.Data[block_offset] == 0x01)
			tmpstr = "Checkmac miscompare";
		else if (block.Data[block_offset] == 0x03)
			tmpstr = "Parse error";
		else if (block.Data[block_offset] == 0x0F)
			tmpstr = "Execution error";
		else if (block.Data[block_offset] == 0x11)
			tmpstr = "Wake received properly";
		else if (block.Data[block_offset] == 0xFF)
			tmpstr = "CRC or communication error";
		else
			tmpstr = "Unknown status code";

		texts.push_back(std::string(param->Name) + " " + tmpstr + " (" + number_str + ")" );
		texts.push_back(std::string(param->Name) + " " + tmpstr);
		texts.push_back(std::string(param->Name) + " (" + number_str + ")");
		texts.push_back(tmpstr);
		texts.push_back(param->Name);

		break;

	case PT_Word:
	case PT_DWord:
		if (param->Type == PT_Word)
		{
			size_t val = (block.Data[block_offset + 1] << 8)  |  block.Data[block_offset];
			AnalyzerHelpers::GetNumberString(val, display_base, 16, number_str, BUFF_SIZE);
		} else {
			size_t val = (block.Data[block_offset + 3] << 24)
							| (block.Data[block_offset + 2] << 16)
							| (block.Data[block_offset + 1] << 8)
							|  block.Data[block_offset];
			AnalyzerHelpers::GetNumberString(val, display_base, 32, number_str, BUFF_SIZE);
		}

		texts.push_back(std::string(param->Name) + " (" + number_str + ")");
		texts.push_back(param->Name);
		texts.push_back(number_str);
		break;
	}
}

void AtmelSWIAnalyzerResults::GetTextsForChecksumFrame(const Frame& f, DisplayBase display_base, std::vector<std::string>& texts)
{
	const int BUFF_SIZE = 128;
	char number_str[BUFF_SIZE];

	AnalyzerHelpers::GetNumberString(f.mData1, display_base, 16, number_str, BUFF_SIZE);

	texts.clear();

	if (f.mData1 == f.mData2)
	{
		texts.push_back("Checksum OK (" + std::string(number_str) + ")");
	} else {
		char calc_crc_str[BUFF_SIZE];
		AnalyzerHelpers::GetNumberString(f.mData2, display_base, 16, calc_crc_str, BUFF_SIZE);

		texts.push_back("Checksum Incorrect (" + std::string(number_str) + ") Calculated (" + calc_crc_str + ")");
		texts.push_back("Checksum Incorrect (" + std::string(number_str) + ")");
	}

	texts.push_back(number_str);
}

void AtmelSWIAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
	const int BUFF_SIZE = 128;
	char number_str[BUFF_SIZE];
	ClearResultStrings();
	Frame f = GetFrame(frame_index);

	if (f.mType == FrameToken)
	{
		if (f.mData1 == SWI_Wake)
		{
			AddResultString("Wake");
			AddResultString("Token Wake");
		} else if (f.mData1 == SWI_One) {
			AddResultString("1");
			AddResultString("One");
			AddResultString("Token One");
		} else if (f.mData1 == SWI_Zero) {
			AddResultString("0");
			AddResultString("Zero");
			AddResultString("Token Zero");
		}

	} else if (f.mType == FrameByte) {
		AnalyzerHelpers::GetNumberString(f.mData1, display_base, 8, number_str, BUFF_SIZE);
		AddResultString(number_str);

	} else if (f.mType == FrameChecksum) {

		std::vector<std::string> texts;
		GetTextsForChecksumFrame(f, display_base, texts);

		std::vector<std::string>::iterator i(texts.begin());
		while (i != texts.end())
		{
			AddResultString(i->c_str());
			++i;
		}

	} else if (f.mType == FrameFlag) {
		AnalyzerHelpers::GetNumberString(f.mData1, display_base, 8, number_str, BUFF_SIZE);
		AddResultString(number_str);
		if (f.mData2)
		{
			const char* FlagName = GetFlagName(static_cast<SWI_Flag>(f.mData1));
			AddResultString(FlagName);
			AddResultString("Flag ", FlagName, " (", number_str, ")");
		} else {
			AddResultString("Bad Flag (", number_str, ")");
		}

	} else if (f.mType == FrameCount) {
		AnalyzerHelpers::GetNumberString(f.mData1, display_base, 8, number_str, BUFF_SIZE);
		AddResultString(number_str);
		AddResultString("Count (", number_str, ")");

	} else if (f.mType == FramePacketSegment) {

		std::vector<std::string> texts;
		GetTextsForPacketSegmentFrame(f, display_base, texts);

		std::vector<std::string>::iterator i(texts.begin());
		while (i != texts.end())
		{
			AddResultString(i->c_str());
			++i;
		}
	}
}

void AtmelSWIAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id)
{
	std::ofstream file_stream(file, std::ios::out);

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	Frame f;
	char time_str[128], number_str[128];
	const U64 num_frames = GetNumFrames();
	std::vector<std::string> texts;
	for (U64 fcnt = 0; fcnt < num_frames; fcnt++)
	{
		// get the frame
		f = GetFrame(fcnt);

		// make the time string
		AnalyzerHelpers::GetTimeString(f.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, sizeof(time_str));

		switch (f.mType)
		{
		case FrameToken:
			file_stream << "Token ";

			if (f.mData1 == SWI_Wake)
				file_stream << "Wake";
			else if (f.mData1 == SWI_One)
				file_stream << "One";
			else if (f.mData1 == SWI_Zero)
				file_stream << "Zero";

			file_stream << " at " << time_str << std::endl << std::endl;

			break;
		case FrameByte:
			AnalyzerHelpers::GetNumberString(f.mData1, display_base, 8, number_str, sizeof(number_str));
			file_stream << "Byte (" << number_str << ") at " << time_str << std::endl;
			break;
		case FrameFlag:
			AnalyzerHelpers::GetNumberString(f.mData1, display_base, 8, number_str, sizeof(number_str));
			file_stream << "Flag " << GetFlagName(static_cast<SWI_Flag>(f.mData1)) 
						<< " (" << number_str << ") at " << time_str << std::endl << std::endl;
			break;
		case FrameCount:
			// start the IO block section and write the count byte
			AnalyzerHelpers::GetNumberString(f.mData1, display_base, 8, number_str, sizeof(number_str));
			file_stream << "I/O Block at " << time_str << std::endl;
			file_stream << "Count (" << number_str << ")" << std::endl;

			break;
		case FramePacketSegment:

			// get the packet segment texts
			GetTextsForPacketSegmentFrame(f, display_base, texts);
			file_stream << texts.front() << std::endl;

			break;
		case FrameChecksum:

			// get the checksum texts
			GetTextsForChecksumFrame(f, display_base, texts);
			file_stream << texts.front() << std::endl << std::endl;

			break;
		}

		if (UpdateExportProgressAndCheckForCancel(fcnt, num_frames))
			return;
	}

	// end
	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
}

void AtmelSWIAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
	/*Frame frame = GetFrame(frame_index);
	ClearResultStrings();

	char number_str[128];
	AnalyzerHelpers::GetNumberString(frame.mData1, display_base, 8, number_str, sizeof(number_str));
	AddResultString(number_str);*/

    ClearTabularText();

	const int BUFF_SIZE = 128;
	char number_str[BUFF_SIZE];

	Frame f = GetFrame(frame_index);

	if (f.mType == FrameToken)
	{
		if (f.mData1 == SWI_Wake)
		{
			AddTabularText("Token Wake");
		} else if (f.mData1 == SWI_One) {
			AddTabularText("Token One");
		} else if (f.mData1 == SWI_Zero) {
			AddTabularText("Token Zero");
		}

	} else if (f.mType == FrameByte) {
		AnalyzerHelpers::GetNumberString(f.mData1, display_base, 8, number_str, BUFF_SIZE);
		AddTabularText(number_str);

	} else if (f.mType == FrameChecksum) {

		std::vector<std::string> texts;
		GetTextsForChecksumFrame(f, display_base, texts);

        AddTabularText( texts[0].c_str() );

	} else if (f.mType == FrameFlag) {
		AnalyzerHelpers::GetNumberString(f.mData1, display_base, 8, number_str, BUFF_SIZE);
        AddTabularText("Bad Flag (", number_str, ")");

	} else if (f.mType == FrameCount) {
		AnalyzerHelpers::GetNumberString(f.mData1, display_base, 8, number_str, BUFF_SIZE);
		AddTabularText("Count (", number_str, ")");

	} else if (f.mType == FramePacketSegment) {

		std::vector<std::string> texts;
		GetTextsForPacketSegmentFrame(f, display_base, texts);
        AddTabularText( texts[0].c_str() );
	}
}

void AtmelSWIAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}

void AtmelSWIAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}
