#ifndef RFFE_ANALYZER_SETTINGS
#define RFFE_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class RFFEAnalyzerSettings : public AnalyzerSettings
{
public:
	RFFEAnalyzerSettings();
	virtual ~RFFEAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mSclkChannel;
	Channel mSdataChannel;
	bool    mShowParityInReport;
	bool    mShowBusParkInReport;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel > mSclkChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceChannel > mSdataChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceBool >	 mShowParityInReportInterface;
	std::auto_ptr< AnalyzerSettingInterfaceBool >	 mShowBusParkInReportInterface;
};

#endif //RFFE_ANALYZER_SETTINGS
