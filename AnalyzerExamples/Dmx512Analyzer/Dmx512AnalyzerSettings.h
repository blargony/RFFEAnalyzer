#ifndef DMX512_ANALYZER_SETTINGS
#define DMX512_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class Dmx512AnalyzerSettings : public AnalyzerSettings
{
public:
	Dmx512AnalyzerSettings();
	virtual ~Dmx512AnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	
	Channel mInputChannel;
	U32 mBitRate;
	double mMinMAB;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel > mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceBool > mOldVersionInterface;
};

#endif //DMX512_ANALYZER_SETTINGS
