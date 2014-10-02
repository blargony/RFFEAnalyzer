#include <AnalyzerHelpers.h>

#include "AtmelSWIAnalyzerSettings.h"
#include "AtmelSWIAnalyzerResults.h"
#include "AtmelSWITypes.h"

#define CHANNEL_NAME		"SWI SDA"

AtmelSWIAnalyzerSettings::AtmelSWIAnalyzerSettings()
:	mSDAChannel(UNDEFINED_CHANNEL)
{
	// init the interface
	mSDAChannelInterface.SetTitleAndTooltip(CHANNEL_NAME, "Single Wire Interface SDA");
	mSDAChannelInterface.SetChannel(mSDAChannel);

	mDecodeLevelInterface.SetTitleAndTooltip("Decode level", "Level of the communication to decode");
	mDecodeLevelInterface.AddNumber(DL_Tokens, "Tokens", "Decode only the level of tokens");
	mDecodeLevelInterface.AddNumber(DL_Bytes, "Bytes", "Group the tokens into bytes");
	mDecodeLevelInterface.AddNumber(DL_Packets, "Packets", "Decode the packet contents");
	
	// set default
	mDecodeLevelInterface.SetNumber(DL_Packets);

	// add the interface
	AddInterface(&mSDAChannelInterface);
	AddInterface(&mDecodeLevelInterface);

	// describe export
	AddExportOption(0, "Export as text file");
	AddExportExtension(0, "text", "txt");

	ClearChannels();

	AddChannel(mSDAChannel,	CHANNEL_NAME, false);
}

AtmelSWIAnalyzerSettings::~AtmelSWIAnalyzerSettings()
{}

bool AtmelSWIAnalyzerSettings::SetSettingsFromInterfaces()
{
	if (mSDAChannelInterface.GetChannel() == UNDEFINED_CHANNEL)
	{
		SetErrorText("Please select an input for the SDA channel.");
		return false;
	}

	mSDAChannel = mSDAChannelInterface.GetChannel();
	mDecodeLevel = static_cast<DecodeLevel>(static_cast<int>(mDecodeLevelInterface.GetNumber()));

	ClearChannels();

	AddChannel(mSDAChannel,	CHANNEL_NAME, true);

	return true;
}

void AtmelSWIAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mSDAChannelInterface.SetChannel(mSDAChannel);
	mDecodeLevelInterface.SetNumber(mDecodeLevel);
}

void AtmelSWIAnalyzerSettings::LoadSettings(const char* settings)
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mSDAChannel;
	int temp_dl;
	text_archive >> temp_dl;
	mDecodeLevel = static_cast<DecodeLevel>(temp_dl);

	ClearChannels();

	AddChannel(mSDAChannel, CHANNEL_NAME,	true);

	UpdateInterfacesFromSettings();
}

const char* AtmelSWIAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mSDAChannel;
	text_archive << mDecodeLevel;

	return SetReturnString(text_archive.GetString());
}
