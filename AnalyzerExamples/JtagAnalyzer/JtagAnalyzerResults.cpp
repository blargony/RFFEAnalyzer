#include <iostream>
#include <fstream>
#include <algorithm>

#include <AnalyzerHelpers.h>

#include "JtagAnalyzerResults.h"
#include "JtagAnalyzer.h"
#include "JtagAnalyzerSettings.h"

const char* TAPStateDescLong[] = 
{
	"Test-Logic-Reset",
	"Run-Test/Idle",

	"Select-DR-Scan",
	"Capture-DR",
	"Shift-DR",
	"Exit1-DR",
	"Pause-DR",
	"Exit2-DR",
	"Update-DR",

	"Select-IR-Scan",
	"Capture-IR",
	"Shift-IR",
	"Exit1-IR",
	"Pause-IR",
	"Exit2-IR",
	"Update-IR"
};

const char* TAPStateDescShort[] = 
{
	"TstLogRst",
	"RunTstIdl",

	"SelDRScn",
	"CapDR",
	"ShDR",
	"Ex1DR",
	"PsDR",
	"Ex2DR",
	"UpdDR",

	"SelIRScn",
	"CapIR",
	"ShIR",
	"Ex1IR",
	"PsIR",
	"Ex2IR",
	"UpdIR"
};

JtagAnalyzerResults::JtagAnalyzerResults(JtagAnalyzer* analyzer, JtagAnalyzerSettings* settings)
:	mSettings(settings),
	mAnalyzer(analyzer)
{}

JtagAnalyzerResults::~JtagAnalyzerResults()
{}

void JtagAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
	ClearResultStrings();
	Frame f = GetFrame(frame_index);

	if (channel == mSettings->mTmsChannel)
	{
		// add the TAP state descriptions to the TMS channel
		AddResultString(GetStateDescLong((JtagTAPState) f.mType));
		AddResultString(GetStateDescShort((JtagTAPState) f.mType));

	} else if (channel == mSettings->mTdiChannel  ||  channel == mSettings->mTdoChannel) {

		// find this frame's TDI/TDO data
		JtagShiftedData sd;
		sd.mStartSampleIndex = f.mStartingSampleInclusive;
		std::set<JtagShiftedData>::iterator sdi(mShiftedData.find(sd));

		// found?
		if (sdi != mShiftedData.end())
			AddResultString(channel == mSettings->mTdiChannel ? sdi->GetTDIString(display_base).c_str() : sdi->GetTDOString(display_base).c_str());
	}
}

void JtagAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id)
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s];TAP state;TDI;TDO" << std::endl;

	Frame frm;
	JtagShiftedData sd;
	std::set<JtagShiftedData>::iterator sdi;
	char time_str[128];
	std::string tdi_str, tdo_str;
	const U64 num_frames = GetNumFrames();
	for (U64 fcnt = 0; fcnt < num_frames; fcnt++)
	{
		// get the frame
		frm = GetFrame(fcnt);

		// make the time string
		AnalyzerHelpers::GetTimeString(frm.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, sizeof(time_str));

		// get the TAP state
		JtagTAPState tap_state = (JtagTAPState) frm.mType;

		// make the TDI/TDO data if we're in a shift state
		tdi_str.clear();
		tdo_str.clear();
		if (tap_state == ShiftIR  ||  tap_state == ShiftDR)
		{
			// find TDI/TDO data
			sd.mStartSampleIndex = frm.mStartingSampleInclusive;
			sdi = mShiftedData.find(sd);

			// found?
			if (sdi != mShiftedData.end())
			{
				tdi_str = sdi->GetTDIString(display_base);
				tdo_str = sdi->GetTDOString(display_base);
			}
		}

		// output
		file_stream << time_str << ";" << GetStateDescLong(tap_state) << ";" << tdi_str << ";" << tdo_str << std::endl;

		if (UpdateExportProgressAndCheckForCancel(fcnt, num_frames))
			return;
	}

	// end
	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
}

void JtagAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
	Frame frame = GetFrame(frame_index);
	ClearResultStrings();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	AddResultString( number_str );
}

void JtagAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}

void JtagAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}

void JtagAnalyzerResults::AddShiftedData(const JtagShiftedData& shifted_data)
{
	mShiftedData.insert(shifted_data);
}

const char* JtagAnalyzerResults::GetStateDescLong(const JtagTAPState mCurrTAPState)
{
	if (mCurrTAPState > UpdateIR)
		return "<undefined>";

	return TAPStateDescLong[mCurrTAPState];
}

const char* JtagAnalyzerResults::GetStateDescShort(const JtagTAPState mCurrTAPState)
{
	if (mCurrTAPState > UpdateIR)
		return "<undef>";

	return TAPStateDescShort[mCurrTAPState];
}
