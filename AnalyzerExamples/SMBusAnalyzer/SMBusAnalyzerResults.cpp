#include <cassert>

#include <iostream>
#include <fstream>
#include <algorithm>

#include <AnalyzerHelpers.h>

#include "SMBusAnalyzerResults.h"
#include "SMBusAnalyzer.h"
#include "SMBusAnalyzerSettings.h"
#include "SMBusCommands.h"

SMBusAnalyzerResults::SMBusAnalyzerResults(SMBusAnalyzer* analyzer, SMBusAnalyzerSettings* settings)
:	mSettings(settings),
	mAnalyzer(analyzer)
{}

SMBusAnalyzerResults::~SMBusAnalyzerResults()
{}

struct BitFieldDesc
{
	int			bit_number;
	const char*	bit_name;
};

const BitFieldDesc BatteryModeBits[] = 
{
	{ 0, "INTERNAL_CHARGE_CONTROLLER"},
	{ 1, "PRIMARY_BATTERY_SUPPORT"},
	{ 7, "CONDITION_FLAG"},
	{ 8, "CHARGE_CONTROLLER_ENABLED"},
	{ 9, "PRIMARY_BATTERY"},
	{13, "ALARM_MODE"},
	{14, "CHARGER_MODE"},
	{15, "CAPACITY_MODE"},

	{0, 0}
};

const BitFieldDesc BatteryStatusBits[] = 
{
	{15, "OVER CHARGED ALARM"},
	{14, "TERMINATE CHARGE ALARM"},
	{12, "OVER TEMP ALARM"},
	{11, "TERMINATE DISCHARGE ALARM"},
	{9, "REMAINING CAPACITY ALARM"},
	{8, "REMAINING TIME ALARM"},
	{7, "INITIALIZED"},
	{6, "DISCHARGING"},
	{5, "FULLY CHARGED"},
	{4, "FULLY DISCHARGED"},

	{0, 0}
};

const char* ErrorCodesDesc[] = 
{
	"OK",
	"Busy",
	"ReservedCommand",
	"UnsupportedCommand",
	"AccessDenied",
	"Overflow/Underflow",
	"BadSize",
	"UnknownError",
};

void BitFieldToDescAll(const BitFieldDesc* bfld, U64 val, std::vector<std::string>& desc)
{
	desc.clear();

	while (bfld->bit_name)
	{
		desc.push_back(bfld->bit_name);
		desc.back() += "=";

		if (val & U16(1 << bfld->bit_number))
			desc.back() += "1";
		else
			desc.back() += "0";

		++bfld;
	}
}

void SMBusAnalyzerResults::AddResultStringsFromVector(const std::string& prefix, const std::vector<std::string>& v, const std::string& postfix, std::vector<std::string>& results)
{
	results.clear();

	results.push_back(prefix + "...");

	std::string str;
	for (std::vector<std::string>::const_iterator vi(v.begin());
				vi != v.end(); ++vi)
	{
		str += *vi;

		if (vi + 1 < v.end()  ||  !postfix.empty())
		{
			str += ", ";
			results.push_back(prefix + str + "...");
		} else {
			results.push_back(prefix + str);
		}
	}

	if (!postfix.empty())
		results.push_back(prefix + str + postfix);
}

void SMBusAnalyzerResults::GetBubbleText(const Frame& f, const bool isClock, DisplayBase display_base, std::vector<std::string>& results)
{
	results.clear();

    if (f.mType == FT_Desc)
	{
		if (isClock)
		{
			std::string protName = "Unknown";
			std::string protSpec = "Unknown";

			if (f.mData1 != 0)
			{
				const SMBusProtocol* pProt = (const SMBusProtocol*) f.mData1;

				protName = pProt->name;

				if (pProt >= SMBusProtocols  &&  pProt < (SMBusProtocols + NUM_SMBUS_PROTOCOLS))
					protSpec = "SMBus ";
				else
					protSpec = "PMBus ";
			} else if (f.mData2 == 1) {		// PMBus group command
				protName = "Group command";
				protSpec = "PMBus ";
			} else if (f.mData2 == 2) {		// raw data
				protName = "";
				protSpec = "Unable to match ";
			}

			results.push_back(protSpec + "protocol " + protName);
			results.push_back(protSpec + protName);
			results.push_back(protName);
			results.push_back(protSpec);
		}

	} else if (!isClock) {

		if (f.mType == SMB_Start)
		{
			if (mSettings->mDecodeLevel == DL_Signals)
				results.push_back("Start");
		} else if (f.mType == SMB_Stop) {
			if (mSettings->mDecodeLevel == DL_Signals)
				results.push_back("Stop");
		} else if (f.mType == SMB_Zero)
			results.push_back("0");
		else if (f.mType == SMB_One)
			results.push_back("1");
		else if (f.mType == SMB_ACK)
			results.push_back("ACK");
		else if (f.mType == SMB_NACK)
			results.push_back("NACK");
		else if (f.mType == FT_Byte) {

			std::string num(int2str_sal(f.mData1, display_base, 8));
			results.push_back("Byte " + num + ((f.mFlags & F_IsAcked) ? " ACK" : " NACK"));
			results.push_back("Byte " + num + ((f.mFlags & F_IsAcked) ? " A" : " N"));
			results.push_back(num + ((f.mFlags & F_IsAcked) ? " A" : " N"));
			results.push_back(num);

		} else if (f.mType == FT_Word) {

			std::string num(int2str_sal(f.mData1, display_base, 16));

			results.push_back("Word " + num + ((f.mFlags & F_IsAcked) ? " ACK" : " NACK"));
			results.push_back("Word " + num + ((f.mFlags & F_IsAcked) ? " A" : " N"));
			results.push_back(num + ((f.mFlags & F_IsAcked) ? " A" : " N"));
			results.push_back(num);

		} else if (f.mType == FT_PEC) {

			std::string readPEC(int2str_sal(f.mData1, display_base, 8));
			std::string calcedPEC(int2str_sal(f.mData2, display_base, 8));

			if (f.mData1 == f.mData2)
			{
				results.push_back("PEC " + readPEC + " OK");
				results.push_back(readPEC + " OK");
				results.push_back("PEC OK");
			} else {
				results.push_back("Bad PEC " + readPEC + " should be " + calcedPEC);
				results.push_back(readPEC + " bad PEC!");
				results.push_back("Bad PEC!");
			}

			results.push_back(readPEC);

		} else if (f.mType == FT_Address) {

			std::string addr(int2str_sal(f.mData1, display_base, 7));
			std::string rw = (f.mFlags & F_IsRead) ? "Read " : "Write ";
			std::string rw_s = (f.mFlags & F_IsRead) ? "R " : "W ";
			const bool is_acked = f.mFlags & F_IsAcked;

			results.push_back(rw + "address " + addr + (is_acked ? " ACK" : " NACK"));
			results.push_back(addr);
			results.push_back(addr + (is_acked ? " A" : " N"));
			results.push_back(rw + addr + (is_acked ? " A" : " N"));
			results.push_back(rw + addr + (is_acked ? " ACK" : " NACK"));
			results.push_back(rw_s + addr + (is_acked ? " A" : " N"));
			results.push_back(rw_s + addr + (is_acked ? " ACK" : " NACK"));
			results.push_back(rw_s + "addr " + addr + (is_acked ? " A" : " N"));
			results.push_back(rw_s + "address " + addr + (is_acked ? " ACK" : " NACK"));
			results.push_back(rw + "addr " + addr + (is_acked ? " A" : " N"));
			results.push_back(rw + "address " + addr + (is_acked ? " ACK" : " NACK"));

		} else if (f.mType == FT_ByteCount) {

			std::string bc_str(int2str_sal(f.mData1, display_base, 7));

			results.push_back("Byte count " + bc_str);
			results.push_back("ByteCount " + bc_str);
			results.push_back("Cnt " + bc_str);
			results.push_back(bc_str);

		} else if (f.mType == FT_CmdPMBus  ||  f.mType == FT_CmdSmartBattery) {

			CommandDesc cmd;
			if (f.mType == FT_CmdPMBus)
				cmd = GetPMBusCommandDesc(f.mData1);
			else
				cmd = GetSmartBatteryCommandDesc(f.mData1);

			std::string id(int2str_sal(f.mData1, display_base, 8));

			results.push_back("Command " + std::string(cmd.name) + " " + id);
			results.push_back(id);
			results.push_back(cmd.name);
			results.push_back("Cmd " + std::string(cmd.name));
			results.push_back("Cmd " + id);
			results.push_back("Cmd " + std::string(cmd.name) + " " + id);
			results.push_back("Command " + std::string(cmd.name));
			results.push_back("Command " + id);

		} else if (f.mType == FT_CmdSMBus) {

			std::string id(int2str_sal(f.mData1, display_base, 8));

			results.push_back("Command " + id);
			results.push_back(id);
			results.push_back("Cmd " + id);

		} else if (f.mType == FT_PMBusCapability) {

			U8 bm = (U8) f.mData1;
			const char* pec_desc = (f.mData1 & 0x80 ? "PEC unsupported, " : "PEC supported, ");

			const char* max_bus_speed;
			switch (f.mData1 & 0x60)
			{
			case 0x00: max_bus_speed = "Max bus speed 100KHz, "; break;
			case 0x20: max_bus_speed = "Max bus speed 400KHz, "; break;
			default: max_bus_speed = "Reserved, "; break;
			}

			const char* smbalert_desc = ((f.mData1 & 0x10) ? "SMBALERT supported " : "SMBALERT unsupported ");

			std::string val_str(int2str_sal(f.mData1, display_base, 8));

			results.push_back("Capability data byte " + val_str + " " + pec_desc + max_bus_speed + smbalert_desc);
			results.push_back(val_str);
			results.push_back("Capability " + val_str + " ...");
			results.push_back("Capability " + val_str + " " + pec_desc + "...");
			results.push_back("Capability " + val_str + " " + pec_desc + max_bus_speed + "...");

		} else if (f.mType == FT_PMBusQuery) {
			
			U8 bm = (U8) f.mData1;

			// command supported?
			if (bm & 0x80)
			{
				std::string for_write = (bm & 0x40) ? "for write" : "";
				const char* for_read = (bm & 0x20) ? "for read" : "";
				const char* format;
				switch (bm & 0x1C)
				{
				case 0x00:	format = "Linear data format"; break;
				case 0x0C:	format = "Direct mode format"; break;
				case 0x14:	format = "VID mode format"; break;
				case 0x18:	format = "Manufacturer specific format"; break;
				default:	format = "error format"; break;
				}

				results.push_back("Command supported " + for_write + for_read + format);
				results.push_back("Command supported " + for_write + for_read + "...");
				results.push_back("Command supported " + for_write + "...");
				results.push_back("Command supported...");
				results.push_back("Supported...");
			} else {
				results.push_back("Command unsupported");
				results.push_back("unsupported");
			}

		} else if (f.mType == FT_PMBusWriteProtect) {

			switch (f.mData1)
			{
			case 0x80:
				results.push_back("Disable all writes except to the WRITE_PROTECT command");
				results.push_back("Disable...");
				results.push_back("Disable all except WRITE_PROTECT...");
				break;
			case 0x40:
				results.push_back("Disable all writes except to the WRITE_PROTECT, OPERATION and PAGE commands");
				results.push_back("Disable...");
				results.push_back("Disable all except WRITE_PROTECT, OPERATION and PAGE...");
				break;
			case 0x20:
				results.push_back("Disable all writes except to the WRITE_PROTECT, OPERATION, PAGE, ON_OFF_CONFIG and VOUT_COMMAND commands");
				results.push_back("Disable...");
				results.push_back("Disable all except WRITE_PROTECT, OPERATION, PAGE, ON_OFF_CONFIG and VOUT_COMMAND...");
				break;
			case 0x00:
				results.push_back("Enable writes to all commands");
				results.push_back("Enable...");
				results.push_back("Enable all writes...");
				break;
			default:
				results.push_back("Invalid Data!");
				results.push_back("Invalid");
				break;
			}

		} else if (f.mType == FT_PMBusOperation) {

			std::string unit_on_off = "<invalid>";
			const char* margin_state = "<invalid>";
			switch (f.mData1 & 0xC0)
			{
			case 0x00:
				unit_on_off = "Immediate off (no sequencing)";
				margin_state = "N/A";
				break;
			case 0x40:
				unit_on_off = "Soft off (with sequencing)";
				margin_state = "N/A";
				break;
			case 0x80:
				unit_on_off = "Unit on";
				switch (f.mData1 & 0x3C)
				{
				case 0x14:	margin_state = "Margin low (ignore fault)";		break;
				case 0x18:	margin_state = "Margin low (act on fault)";		break;
				case 0x24:	margin_state = "Margin high (ignore fault)";	break;
				case 0x28:	margin_state = "Margin high (act on fault)";	break;
				}
				break;
			}

			std::string val(int2str_sal(f.mData1, display_base, 8));

			results.push_back("Operation params " + val + " Unit on/off=" + unit_on_off + ", Margin state=" + margin_state);
			results.push_back(val);
			results.push_back("Operation params...");
			results.push_back(unit_on_off + ", " + margin_state);

		} else if (f.mType == FT_PMBusOnOffConfig) {

			std::string val(int2str_sal(f.mData1, display_base, 8));

			const char* power_up = (f.mData1 & 0x10) ? "CONTROL pin and OPERATION command" : "Any time";
			const char* commands = (f.mData1 & 0x08) ? "Acknowledge on/off portion of OPERATION command" : "Ignore on/off portion of OPERATION command";
			const char* control  = (f.mData1 & 0x04) ? "CONTROL pin required to operate" : "Ignore CONTROL pin";
			const char* polarity = (f.mData1 & 0x02) ? "CONTROL is active high" : "CONTROL is active low";
			const char* action   = (f.mData1 & 0x01) ? "Turn off the output as fast as possible" : "Use programmed turn off delay";

			results.push_back("Params " + val + ": " + power_up + ", " + commands + ", " + polarity + ", " + action);
			results.push_back(val);
			results.push_back("Params " + val + ": " + power_up);
			results.push_back("Params " + val + ": " + power_up + "...");
			results.push_back("Params " + val + ": " + power_up + ", " + commands + "...");
			results.push_back("Params " + val + ": " + power_up + ", " + commands + ", " + polarity + "...");

		} else if (f.mType == FT_PMBusVoutMode) {

			std::string val(int2str_sal(f.mData1, display_base, 8));

			U8 mode = U8(f.mData1 >> 5);
			U8 param = U8(f.mData1 & 0x1F);

			results.push_back(val + " Mode: "+ int2str_sal(mode, display_base, 3) + " Param: " + int2str_sal(param, display_base, 5));
			results.push_back(val);

		} else if (f.mType == FT_SmartBattBatteryMode) {

			std::string val(int2str_sal(f.mData1, display_base, 16));
			std::string prefix("BatteryMode " + val + " ");

			std::vector<std::string> v;
			BitFieldToDescAll(BatteryModeBits, f.mData1, v);

			AddResultStringsFromVector(prefix, v, "", results);

			// make sure the longest description is the first in the vector
			std::swap(results.front(), results.back());

			results.push_back(val);

		} else if (f.mType == FT_SmartBattBatteryStatus) {

			std::string val(int2str_sal(f.mData1, display_base, 16));
			std::string prefix("BatteryStatus " + val + " ");

			std::vector<std::string> v;
			BitFieldToDescAll(BatteryStatusBits, f.mData1, v);

			AddResultStringsFromVector(prefix, v, "ErrorCode=" + std::string(ErrorCodesDesc[f.mData1 & 0x07]), results);

			// make sure the longest description is the first in the vector
			std::swap(results.front(), results.back());

			results.push_back(val);

		} else if (f.mType == FT_SmartBattSpecificationInfo) {

			std::string val(int2str_sal(f.mData1, display_base, 16));

			U8 revision	= f.mData1 & 0x0f;
			U8 version	= (f.mData1 >> 4) & 0x0f;
			U8 vscale	= (f.mData1 >> 8) & 0x0f;
			U8 ipscale	= U8(f.mData1 >> 12);

			results.push_back("SpecificationInfo " + val + " Revision=" + int2str_sal(revision, display_base, 4)
															+ " Version=" + int2str_sal(version, display_base, 4)
															+ " VScale=" + int2str_sal(vscale, display_base, 4)
															+ " IPScale=" + int2str_sal(ipscale, display_base, 4));

			results.push_back("SpecificationInfo " + val + " Revision=" + int2str_sal(revision, display_base, 4) + ", ...");
			
			results.push_back("SpecificationInfo " + val + " Revision=" + int2str_sal(revision, display_base, 4)
															+ " Version=" + int2str_sal(version, display_base, 4) + ", ...");
			
			results.push_back("SpecificationInfo " + val + " Revision=" + int2str_sal(revision, display_base, 4)
															+ " Version=" + int2str_sal(version, display_base, 4)
															+ " VScale=" + int2str_sal(vscale, display_base, 4) + ", ...");

			results.push_back(val);
			results.push_back("SpecificationInfo " + val + " ...");

		} else if (f.mType == FT_SmartBattManufactureDate) {

			std::string val(int2str_sal(f.mData1, display_base, 16));

			U8 day		= f.mData1 & 0x1f;			// 5 bits
			U8 month	= (f.mData1 >> 5) & 0x0f;	// 4 bits
			U16 year	= (f.mData1 >> 9) & 0x9f;	// 7 bits
			year += 1980;
		
			std::string date(int2str(month) + "/" + int2str(day) + "/" + int2str(year));

			results.push_back("ManufactureDate " + val + " " + date);
			results.push_back("ManufactureDate " + val + " ...");
			results.push_back(val);

		} else {
			results.push_back(".");
		}
	}

}

void SMBusAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
	ClearResultStrings();
	Frame f = GetFrame(frame_index);

	std::vector<std::string> results;
	GetBubbleText(f, channel == mSettings->mSMBCLK, display_base, results);

	for (std::vector<std::string>::iterator ri(results.begin()); ri != results.end(); ++ri)
		AddResultString(ri->c_str());
}

void SMBusAnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id)
{
	std::ofstream file_stream(file, std::ios::out);

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	Frame f;
	char time_str[128];
	const U64 num_frames = GetNumFrames();
	std::vector<std::string> results;
	for (U64 fcnt = 0; fcnt < num_frames; fcnt++)
	{
		f = GetFrame(fcnt);		// get the frame

		if (f.mType == FT_Start)
		{
			// make the time string
			AnalyzerHelpers::GetTimeString(f.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, sizeof(time_str));
			file_stream << "Start: " << time_str << std::endl;
		} else if (f.mType == FT_Stop) {
			file_stream << std::endl;
		} else {
			GetBubbleText(f, f.mType == FT_Desc, display_base, results);
			file_stream << results.front() << std::endl;
		}

		if (UpdateExportProgressAndCheckForCancel(fcnt, num_frames))
			return;
	}

	// end
	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
}

void SMBusAnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
    ClearTabularText();
    Frame f = GetFrame(frame_index);
    std::vector<std::string> results;
    bool isClock;
    //if( mSettings->mSMBCLK != UNDEFINED_CHANNEL )
    //    isClock = true;

    if( mSettings->mSMBCLK != UNDEFINED_CHANNEL )
    {
        if (f.mType == FT_Desc)
        {
            //if (isClock)
            {
                std::string protName = "Unknown";
                std::string protSpec = "Unknown";

                if (f.mData1 != 0)
                {
                    const SMBusProtocol* pProt = (const SMBusProtocol*) f.mData1;

                    protName = pProt->name;

                    if (pProt >= SMBusProtocols  &&  pProt < (SMBusProtocols + NUM_SMBUS_PROTOCOLS))
                        protSpec = "SMBus ";
                    else
                        protSpec = "PMBus ";
                } else if (f.mData2 == 1) {		// PMBus group command
                    protName = "Group command";
                    protSpec = "PMBus ";
                } else if (f.mData2 == 2) {		// raw data
                    protName = "";
                    protSpec = "Unable to match ";
                }

                results.push_back(protSpec + "protocol " + protName);
            }

        }
    }

    if ( mSettings->mSMBDAT != UNDEFINED_CHANNEL )
    {

        if (f.mType == SMB_Start)
        {
            if (mSettings->mDecodeLevel == DL_Signals)
                results.push_back("Start");
        } else if (f.mType == SMB_Stop) {
            if (mSettings->mDecodeLevel == DL_Signals)
                results.push_back("Stop");
        } else if (f.mType == SMB_Zero)
            results.push_back("0");
        else if (f.mType == SMB_One)
            results.push_back("1");
        else if (f.mType == SMB_ACK)
            results.push_back("ACK");
        else if (f.mType == SMB_NACK)
            results.push_back("NACK");
        else if (f.mType == FT_Byte) {

            std::string num(int2str_sal(f.mData1, display_base, 8));
            results.push_back("Byte " + num + ((f.mFlags & F_IsAcked) ? " ACK" : " NACK"));
        } else if (f.mType == FT_Word) {

            std::string num(int2str_sal(f.mData1, display_base, 16));

            results.push_back("Word " + num + ((f.mFlags & F_IsAcked) ? " ACK" : " NACK"));

        } else if (f.mType == FT_PEC) {

            std::string readPEC(int2str_sal(f.mData1, display_base, 8));
            std::string calcedPEC(int2str_sal(f.mData2, display_base, 8));

            if (f.mData1 == f.mData2)
            {
                results.push_back("PEC " + readPEC + " OK");
            } else {
                results.push_back("Bad PEC " + readPEC + " should be " + calcedPEC);
            }
        } else if (f.mType == FT_Address) {

            std::string addr(int2str_sal(f.mData1, display_base, 7));
            std::string rw = (f.mFlags & F_IsRead) ? "Read " : "Write ";
            std::string rw_s = (f.mFlags & F_IsRead) ? "R " : "W ";
            const bool is_acked = f.mFlags & F_IsAcked;

            results.push_back(rw + "address " + addr + (is_acked ? " ACK" : " NACK"));

        } else if (f.mType == FT_ByteCount) {

            std::string bc_str(int2str_sal(f.mData1, display_base, 7));

            results.push_back("Byte count " + bc_str);

        } else if (f.mType == FT_CmdPMBus  ||  f.mType == FT_CmdSmartBattery) {

            CommandDesc cmd;
            if (f.mType == FT_CmdPMBus)
                cmd = GetPMBusCommandDesc(f.mData1);
            else
                cmd = GetSmartBatteryCommandDesc(f.mData1);

            std::string id(int2str_sal(f.mData1, display_base, 8));

            results.push_back("Command " + std::string(cmd.name) + " " + id);
        } else if (f.mType == FT_CmdSMBus) {

            std::string id(int2str_sal(f.mData1, display_base, 8));

            results.push_back("Command " + id);

        } else if (f.mType == FT_PMBusCapability) {

            U8 bm = (U8) f.mData1;
            const char* pec_desc = (f.mData1 & 0x80 ? "PEC unsupported, " : "PEC supported, ");

            const char* max_bus_speed;
            switch (f.mData1 & 0x60)
            {
            case 0x00: max_bus_speed = "Max bus speed 100KHz, "; break;
            case 0x20: max_bus_speed = "Max bus speed 400KHz, "; break;
            default: max_bus_speed = "Reserved, "; break;
            }

            const char* smbalert_desc = ((f.mData1 & 0x10) ? "SMBALERT supported " : "SMBALERT unsupported ");

            std::string val_str(int2str_sal(f.mData1, display_base, 8));

            results.push_back("Capability data byte " + val_str + " " + pec_desc + max_bus_speed + smbalert_desc);

        } else if (f.mType == FT_PMBusQuery) {

            U8 bm = (U8) f.mData1;

            // command supported?
            if (bm & 0x80)
            {
                std::string for_write = (bm & 0x40) ? "for write" : "";
                const char* for_read = (bm & 0x20) ? "for read" : "";
                const char* format;
                switch (bm & 0x1C)
                {
                case 0x00:	format = "Linear data format"; break;
                case 0x0C:	format = "Direct mode format"; break;
                case 0x14:	format = "VID mode format"; break;
                case 0x18:	format = "Manufacturer specific format"; break;
                default:	format = "error format"; break;
                }

                results.push_back("Command supported " + for_write + for_read + format);
            } else {
                results.push_back("Command unsupported");
            }

        } else if (f.mType == FT_PMBusWriteProtect) {

            switch (f.mData1)
            {
            case 0x80:
                results.push_back("Disable all writes except to the WRITE_PROTECT command");
                break;
            case 0x40:
                results.push_back("Disable all writes except to the WRITE_PROTECT, OPERATION and PAGE commands");
                break;
            case 0x20:
                results.push_back("Disable all writes except to the WRITE_PROTECT, OPERATION, PAGE, ON_OFF_CONFIG and VOUT_COMMAND commands");
                break;
            case 0x00:
                results.push_back("Enable writes to all commands");
                break;
            default:
                results.push_back("Invalid Data!");
                break;
            }

        } else if (f.mType == FT_PMBusOperation) {

            std::string unit_on_off = "<invalid>";
            const char* margin_state = "<invalid>";
            switch (f.mData1 & 0xC0)
            {
            case 0x00:
                unit_on_off = "Immediate off (no sequencing)";
                margin_state = "N/A";
                break;
            case 0x40:
                unit_on_off = "Soft off (with sequencing)";
                margin_state = "N/A";
                break;
            case 0x80:
                unit_on_off = "Unit on";
                switch (f.mData1 & 0x3C)
                {
                case 0x14:	margin_state = "Margin low (ignore fault)";		break;
                case 0x18:	margin_state = "Margin low (act on fault)";		break;
                case 0x24:	margin_state = "Margin high (ignore fault)";	break;
                case 0x28:	margin_state = "Margin high (act on fault)";	break;
                }
                break;
            }

            std::string val(int2str_sal(f.mData1, display_base, 8));

            results.push_back("Operation params " + val + " Unit on/off=" + unit_on_off + ", Margin state=" + margin_state);

        } else if (f.mType == FT_PMBusOnOffConfig) {

            std::string val(int2str_sal(f.mData1, display_base, 8));

            const char* power_up = (f.mData1 & 0x10) ? "CONTROL pin and OPERATION command" : "Any time";
            const char* commands = (f.mData1 & 0x08) ? "Acknowledge on/off portion of OPERATION command" : "Ignore on/off portion of OPERATION command";
            const char* control  = (f.mData1 & 0x04) ? "CONTROL pin required to operate" : "Ignore CONTROL pin";
            const char* polarity = (f.mData1 & 0x02) ? "CONTROL is active high" : "CONTROL is active low";
            const char* action   = (f.mData1 & 0x01) ? "Turn off the output as fast as possible" : "Use programmed turn off delay";

            results.push_back("Params " + val + ": " + power_up + ", " + commands + ", " + polarity + ", " + action);

        } else if (f.mType == FT_PMBusVoutMode) {

            std::string val(int2str_sal(f.mData1, display_base, 8));

            U8 mode = U8(f.mData1 >> 5);
            U8 param = U8(f.mData1 & 0x1F);

            results.push_back(val + " Mode: "+ int2str_sal(mode, display_base, 3) + " Param: " + int2str_sal(param, display_base, 5));
        } else if (f.mType == FT_SmartBattBatteryMode) {

            std::string val(int2str_sal(f.mData1, display_base, 16));
            std::string prefix("BatteryMode " + val + " ");

            std::vector<std::string> v;
            BitFieldToDescAll(BatteryModeBits, f.mData1, v);

            AddResultStringsFromVector(prefix, v, "", results);

            // make sure the longest description is the first in the vector
            std::swap(results.front(), results.back());

            results.push_back(val);

        } else if (f.mType == FT_SmartBattBatteryStatus) {

            std::string val(int2str_sal(f.mData1, display_base, 16));
            std::string prefix("BatteryStatus " + val + " ");

            std::vector<std::string> v;
            BitFieldToDescAll(BatteryStatusBits, f.mData1, v);

            AddResultStringsFromVector(prefix, v, "ErrorCode=" + std::string(ErrorCodesDesc[f.mData1 & 0x07]), results);

            // make sure the longest description is the first in the vector
            std::swap(results.front(), results.back());

        } else if (f.mType == FT_SmartBattSpecificationInfo) {

            std::string val(int2str_sal(f.mData1, display_base, 16));

            U8 revision	= f.mData1 & 0x0f;
            U8 version	= (f.mData1 >> 4) & 0x0f;
            U8 vscale	= (f.mData1 >> 8) & 0x0f;
            U8 ipscale	= U8(f.mData1 >> 12);

            results.push_back("SpecificationInfo " + val + " Revision=" + int2str_sal(revision, display_base, 4)
                                                            + " Version=" + int2str_sal(version, display_base, 4)
                                                            + " VScale=" + int2str_sal(vscale, display_base, 4)
                                                            + " IPScale=" + int2str_sal(ipscale, display_base, 4));

        } else if (f.mType == FT_SmartBattManufactureDate) {

            std::string val(int2str_sal(f.mData1, display_base, 16));

            U8 day		= f.mData1 & 0x1f;			// 5 bits
            U8 month	= (f.mData1 >> 5) & 0x0f;	// 4 bits
            U16 year	= (f.mData1 >> 9) & 0x9f;	// 7 bits
            year += 1980;

            std::string date(int2str(month) + "/" + int2str(day) + "/" + int2str(year));

            results.push_back("ManufactureDate " + val + " " + date);

        } else {
            results.push_back(".");
        }
    }

    for( int i = 0; i < results.size() ; i++ )
        AddTabularText( results[i].c_str() );
}

void SMBusAnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}

void SMBusAnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}
