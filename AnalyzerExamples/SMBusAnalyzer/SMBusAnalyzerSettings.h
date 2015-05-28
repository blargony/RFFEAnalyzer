#ifndef SMBUS_ANALYZER_SETTINGS_H
#define SMBUS_ANALYZER_SETTINGS_H

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

#include "SMBusTypes.h"

class SMBusAnalyzerSettings : public AnalyzerSettings
{
public:
	SMBusAnalyzerSettings();
	virtual ~SMBusAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	virtual void LoadSettings(const char* settings);
	virtual const char* SaveSettings();

	void UpdateInterfacesFromSettings();

	Channel		mSMBDAT;
	Channel		mSMBCLK;

	SMBusDecodeLevel	mDecodeLevel;

	bool CalcPEC()
	{
		return mCalculatePECInterface.GetValue();
	}

protected:
	AnalyzerSettingInterfaceChannel		mSMBDATInterface;
	AnalyzerSettingInterfaceChannel		mSMBCLKInterface;

	AnalyzerSettingInterfaceNumberList	mDecodeLevelInterface;
	AnalyzerSettingInterfaceBool		mCalculatePECInterface;

};

#endif	// SMBUS_ANALYZER_SETTINGS_H
