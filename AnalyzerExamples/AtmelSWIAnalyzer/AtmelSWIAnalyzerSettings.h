#ifndef ATMEL_SWI_ANALYZER_SETTINGS_H
#define ATMEL_SWI_ANALYZER_SETTINGS_H

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

#include "AtmelSWITypes.h"

// the required level of data stream decoding 
enum DecodeLevel
{
	DL_Tokens,
	DL_Bytes,
	DL_Packets,
};

class AtmelSWIAnalyzerSettings : public AnalyzerSettings
{
public:
	AtmelSWIAnalyzerSettings();
	virtual ~AtmelSWIAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	virtual void LoadSettings(const char* settings);
	virtual const char* SaveSettings();

	void UpdateInterfacesFromSettings();

	Channel		mSDAChannel;
	DecodeLevel	mDecodeLevel;

protected:
	AnalyzerSettingInterfaceChannel		mSDAChannelInterface;
	AnalyzerSettingInterfaceNumberList	mDecodeLevelInterface;
};

#endif	// ATMEL_SWI_ANALYZER_SETTINGS_H
