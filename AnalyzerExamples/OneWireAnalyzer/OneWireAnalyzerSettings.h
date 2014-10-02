#ifndef ONEWIRE_ANALYZER_SETTINGS
#define ONEWIRE_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class OneWireAnalyzerSettings : public AnalyzerSettings
{
public:
	OneWireAnalyzerSettings();
	virtual ~OneWireAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();  //Get the settings out of the interfaces, validate them, and save them to your local settings vars.
	virtual void LoadSettings( const char* settings );  //Load your settings from a string.
	virtual const char* SaveSettings();  //Save your settings to a string.

	void UpdateInterfacesFromSettings();

	Channel mOneWireChannel;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel > mOneWireChannelInterface;

};
#endif //ONEWIRE_ANALYZER_SETTINGS
