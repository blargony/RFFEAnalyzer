#include <AnalyzerHelpers.h>

#include "SMBusAnalyzerSettings.h"
#include "SMBusAnalyzerResults.h"
#include "SMBusTypes.h"

SMBusAnalyzerSettings::SMBusAnalyzerSettings()
:	mSMBDAT(UNDEFINED_CHANNEL),
	mSMBCLK(UNDEFINED_CHANNEL),
	mDecodeLevel(DL_Signals)
{
	// init the interface
	mSMBDATInterface.SetTitleAndTooltip("SMBDAT", "SMBus data line");
	mSMBDATInterface.SetChannel(mSMBDAT);

	mSMBCLKInterface.SetTitleAndTooltip("SMBCLK", "SMBus clock line");
	mSMBCLKInterface.SetChannel(mSMBCLK);

	mDecodeLevelInterface.SetTitleAndTooltip("SMBus decode level", "Type of decoded SMBus data");
	mDecodeLevelInterface.AddNumber(DL_Signals, "Signals", "Decode the signal states");
	mDecodeLevelInterface.AddNumber(DL_Bytes, "Bytes", "Decode the data as raw bytes");
	mDecodeLevelInterface.AddNumber(DL_SMBus, "SMBus", "Decode SMBus basic protocol");
	mDecodeLevelInterface.AddNumber(DL_PMBus, "PMBus", "Decode PMBus commands");
	mDecodeLevelInterface.AddNumber(DL_SmartBattery, "Smart Battery", "Decode Smart Battery commands");

	mDecodeLevelInterface.SetNumber(DL_Signals);

	mCalculatePECInterface.SetValue(true);
	mCalculatePECInterface.SetTitleAndTooltip("Calculate PEC on packets", "true - calculate PEC, false - no PEC on packets");

	// add the interfaces
	AddInterface(&mSMBDATInterface);
	AddInterface(&mSMBCLKInterface);
	AddInterface(&mDecodeLevelInterface);
	AddInterface(&mCalculatePECInterface);

	// describe export
	AddExportOption(0, "Export as text file");
	AddExportExtension(0, "text", "txt");

	ClearChannels();

	AddChannel(mSMBDAT, "SMBDAT", false);
	AddChannel(mSMBCLK, "SMBCLK", false);
}

SMBusAnalyzerSettings::~SMBusAnalyzerSettings()
{}

bool SMBusAnalyzerSettings::SetSettingsFromInterfaces()
{
	if (mSMBDATInterface.GetChannel() == UNDEFINED_CHANNEL)
	{
		SetErrorText("Please select an input for the SMBDAT.");
		return false;
	}

	if (mSMBCLKInterface.GetChannel() == UNDEFINED_CHANNEL)
	{
		SetErrorText("Please select an input for the SMBCLK.");
		return false;
	}

	const int NUM_CHANNELS = 4;
	Channel	all_channels[NUM_CHANNELS] = {mSMBDATInterface.GetChannel(), mSMBCLKInterface.GetChannel()};

	if (AnalyzerHelpers::DoChannelsOverlap(all_channels, NUM_CHANNELS))
	{
		SetErrorText("Please select different inputs for the channels.");
		return false;
	}

	mSMBDAT = mSMBDATInterface.GetChannel();
	mSMBCLK = mSMBCLKInterface.GetChannel();

	ClearChannels();

	AddChannel(mSMBDAT, "SMBDAT", true);
	AddChannel(mSMBCLK, "SMBCLK", true);

	mDecodeLevel = SMBusDecodeLevel(int(mDecodeLevelInterface.GetNumber()));

	return true;
}

void SMBusAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mSMBDATInterface.SetChannel(mSMBDAT);
	mSMBCLKInterface.SetChannel(mSMBCLK);

	mDecodeLevelInterface.SetNumber(mDecodeLevel);
}

void SMBusAnalyzerSettings::LoadSettings(const char* settings)
{
	SimpleArchive text_archive;
	text_archive.SetString(settings);

	text_archive >> mSMBDAT;
	text_archive >> mSMBCLK;

	int s;
	text_archive >> s;
	mDecodeLevel = SMBusDecodeLevel(s);

	ClearChannels();

	AddChannel(mSMBDAT, "SMBDAT", true);
	AddChannel(mSMBCLK, "SMBCLK", true);

	bool calc_pec;
	text_archive >> calc_pec;
	mCalculatePECInterface.SetValue(calc_pec);

	UpdateInterfacesFromSettings();
}

const char* SMBusAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mSMBDAT;
	text_archive << mSMBCLK;
	text_archive << mDecodeLevel;
	text_archive << mCalculatePECInterface.GetValue();

	return SetReturnString(text_archive.GetString());
}
