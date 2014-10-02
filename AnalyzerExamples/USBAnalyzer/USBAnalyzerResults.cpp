#include <iostream>
#include <fstream>
#include <algorithm>

#include <AnalyzerHelpers.h>

#include "USBAnalyzerResults.h"
#include "USBAnalyzer.h"
#include "USBAnalyzerSettings.h"

USBAnalyzerResults::USBAnalyzerResults(USBAnalyzer* analyzer, USBAnalyzerSettings* settings)
:	mSettings(settings),
	mAnalyzer(analyzer)
{}

USBAnalyzerResults::~USBAnalyzerResults()
{}

void GetFrameDesc(const Frame& f, DisplayBase display_base, std::vector<std::string>& results)
{
	results.clear();

	if (f.mType == FT_Signal)
	{
		std::string result;
		if (f.mData1 == S_J)
			result = "J";
		else if (f.mData1 == S_K)
			result = "K";
		else if (f.mData1 == S_SE0)
			result = "SE0";
		else if (f.mData1 == S_SE1)
			result = "SE1";

		results.push_back(result);

	} else if (f.mType == FT_EOP) {
		results.push_back("EOP");
	} else if (f.mType == FT_Reset) {
		results.push_back("Reset");
	} else if (f.mType == FT_Idle) {
		results.push_back("Idle");
	} else if (f.mType == FT_SYNC) {
		results.push_back("SYNC");
	} else if (f.mType == FT_PID) {
		results.push_back("PID " + GetPIDName(USB_PID(f.mData1)));
		results.push_back(GetPIDName(USB_PID(f.mData1)));
	} else if (f.mType == FT_FrameNum) {
		results.push_back("Frame # " + int2str_sal(f.mData1, display_base, 11));
	} else if (f.mType == FT_AddrEndp) {
		results.push_back("Address=" + int2str_sal(f.mData1, display_base, 7) + " Endpoint=" + int2str_sal(f.mData2, display_base, 5));
		results.push_back("Addr=" + int2str_sal(f.mData1, display_base, 7) + " Endp=" + int2str_sal(f.mData2, display_base, 5));
		results.push_back("A:" + int2str_sal(f.mData1, display_base, 7) + " E:" + int2str_sal(f.mData2, display_base, 5));
		results.push_back(int2str_sal(f.mData1, display_base, 7) + " " + int2str_sal(f.mData2, display_base, 5));
	} else if (f.mType == FT_Byte) {
		results.push_back("Byte " + int2str_sal(f.mData1, display_base, 8));
		results.push_back(int2str_sal(f.mData1, display_base, 8));
	} else if (f.mType == FT_KeepAlive) {
		results.push_back("Keep alive");
		results.push_back("KA");
	} else if (f.mType == FT_CRC5  ||  f.mType == FT_CRC16) {
		const int num_bits = f.mType == FT_CRC5 ? 5 : 16;
		results.push_back("CRC");
		if (f.mData1 == f.mData2)
		{
			results.push_back("CRC OK " + int2str_sal(f.mData1, display_base, num_bits));
			results.push_back("CRC OK");
		} else {
			results.push_back("CRC Bad! Rcvd: " + int2str_sal(f.mData1, display_base, num_bits) + " Calc: " + int2str_sal(f.mData2, display_base, num_bits));
			results.push_back("CRC Bad! Rcvd: " + int2str_sal(f.mData1, display_base, num_bits));
			results.push_back("CRC Bad");
		}

		results.push_back(int2str_sal(f.mData1, display_base, num_bits));
	} else if (f.mType == FT_Error) {
		results.push_back("Error packet");
		results.push_back("Error");
		results.push_back("Err");
		results.push_back("E");
	}
}

void USBAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
	ClearResultStrings();
	Frame f = GetFrame(frame_index);
	std::vector<std::string> results;

	GetFrameDesc(f, display_base, results);

	for (std::vector<std::string>::iterator ri(results.begin()); ri != results.end(); ++ri)
		AddResultString(ri->c_str());
}

void USBAnalyzerResults::GenerateExportFilePackets(const char* file, DisplayBase display_base)
{
	std::ofstream file_stream(file, std::ios::out);

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	// header
	file_stream << "Time [s],PID,Address,Endpoint,Frame #,Data,CRC" << std::endl;

	Frame f;
	char time_str[128];
	time_str[0] = '\0';
	const U64 num_frames = GetNumFrames();
	std::string PID, Address, Endpoint, FrameNum, Data, CRC;
	for (U64 fcnt = 0; fcnt < num_frames; fcnt++)
	{
		// get the frame
		f = GetFrame(fcnt);

		if (UpdateExportProgressAndCheckForCancel(fcnt, num_frames))
			return;

		// start of a new packet?
		if (f.mType == FT_SYNC)
		{
			// make the time string
			AnalyzerHelpers::GetTimeString(f.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, sizeof(time_str));

			// reset packet fields
			PID.clear(), Address.clear(), Endpoint.clear(), FrameNum.clear(), Data.clear(), CRC.clear();
		} else if (f.mType == FT_PID) {
			PID = GetPIDName(USB_PID(f.mData1));
		} else if (f.mType == FT_AddrEndp) {
			Address = int2str_sal(f.mData1, display_base, 7);
			Endpoint = int2str_sal(f.mData2, display_base, 5);
		} else if (f.mType == FT_FrameNum) {
			FrameNum = int2str_sal(f.mData1, display_base, 11);
		} else if (f.mType == FT_Byte) {
			Data += (Data.empty() ? "" : " ") + int2str_sal(f.mData1, display_base, 8);
		} else if (f.mType == FT_CRC5  ||  f.mType == FT_CRC16) {
			CRC = int2str_sal(f.mData1, display_base, f.mType == FT_CRC5 ? 5 : 16);
		} else if (f.mType == FT_EOP) {

			// output the packet
			file_stream << time_str << "," << PID << "," << Address << "," << Endpoint << "," << FrameNum << "," << Data << "," << CRC << std::endl;
		}
	}

	// end
	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);}

void USBAnalyzerResults::GenerateExportFileBytes(const char* file, DisplayBase display_base)
{
	std::ofstream file_stream(file, std::ios::out);

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	// header
	file_stream << "Time [s],Byte" << std::endl;

	Frame f;
	char time_str[128];
	time_str[0] = '\0';
	const U64 num_frames = GetNumFrames();
	for (U64 fcnt = 0; fcnt < num_frames; fcnt++)
	{
		// get the frame
		f = GetFrame(fcnt);

		if (UpdateExportProgressAndCheckForCancel(fcnt, num_frames))
			return;

		// start of a new packet?
		if (f.mType == FT_Byte)
		{
			// make the time string
			AnalyzerHelpers::GetTimeString(f.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, sizeof(time_str));

			// output byte and timestamp
			file_stream << time_str << "," << int2str_sal(f.mData1, display_base, 8) << std::endl;
		}
	}

	// end
	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
}

void USBAnalyzerResults::GenerateExportFileSignals(const char* file, DisplayBase display_base)
{
	std::ofstream file_stream(file, std::ios::out);

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	// header
	file_stream << "Time [s],Signal,Duration [ns]" << std::endl;

	Frame f;
	char time_str[128];
	time_str[0] = '\0';
	const U64 num_frames = GetNumFrames();
	for (U64 fcnt = 0; fcnt < num_frames; fcnt++)
	{
		// get the frame
		f = GetFrame(fcnt);

		if (UpdateExportProgressAndCheckForCancel(fcnt, num_frames))
			return;

		// make the time string
		AnalyzerHelpers::GetTimeString(f.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, sizeof(time_str));

		// output timestamp
		file_stream << time_str << ",";

		// start of a new packet?
		if (f.mType == FT_Signal)
		{
			if (f.mData1 == S_J)
				file_stream << 'J';
			else if (f.mData1 == S_K)
				file_stream << 'K';
			else if (f.mData1 == S_SE0)
				file_stream << "SE0";
			else if (f.mData1 == S_SE1)
				file_stream << "SE1";
				
			file_stream << ',' << (f.mEndingSampleInclusive - f.mStartingSampleInclusive) / (sample_rate / 1e9) << std::endl;
		}
	}

	// end
	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
}

void USBAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id)
{
	if (mSettings->mDecodeLevel == OUT_PACKETS)
		GenerateExportFilePackets(file, display_base);
	else if (mSettings->mDecodeLevel == OUT_BYTES)
		GenerateExportFileBytes(file, display_base);
	else if (mSettings->mDecodeLevel == OUT_SIGNALS)
		GenerateExportFileSignals(file, display_base);
}

void USBAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
	Frame f = GetFrame(frame_index);
	std::vector<std::string> results;

	GetFrameDesc(f, display_base, results);

	for (std::vector<std::string>::iterator ri(results.begin()); ri != results.end(); ++ri)
	AddTabularText(ri->c_str());
}

void USBAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}

void USBAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}
