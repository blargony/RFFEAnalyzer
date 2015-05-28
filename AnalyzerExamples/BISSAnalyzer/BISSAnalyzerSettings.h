#ifndef BISS_ANALYZER_SETTINGS
#define BISS_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class BISSAnalyzerSettings : public AnalyzerSettings
{
public:
	BISSAnalyzerSettings();
	virtual ~BISSAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mMaChannel;
	Channel mSloChannel;
	
	//U32 mBitRate;
	U32 mDataLength;

	double mDatenart;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mMaChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mSloChannelInterface;
	//std::auto_ptr< AnalyzerSettingInterfaceInteger >	mBitRateInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mDataLengthInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mDatenartInterface;
};

#endif //BISS_ANALYZER_SETTINGS
