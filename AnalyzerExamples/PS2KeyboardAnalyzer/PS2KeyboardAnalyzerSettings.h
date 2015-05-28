#ifndef PS2KEYBOARD_ANALYZER_SETTINGS
#define PS2KEYBOARD_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class PS2KeyboardAnalyzerSettings : public AnalyzerSettings
{
public:
	PS2KeyboardAnalyzerSettings();
	virtual ~PS2KeyboardAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	Channel mClockChannel;
	Channel mDataChannel;
	double mDeviceType;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mClockChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mDataChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList > mDeviceTypeInterface;
};

#endif //PS2KEYBOARD_ANALYZER_SETTINGS
