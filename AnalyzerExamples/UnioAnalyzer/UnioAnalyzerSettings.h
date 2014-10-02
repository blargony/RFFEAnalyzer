#ifndef UNIO_ANALYZER_SETTINGS
#define UNIO_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

enum MasterAcknowkedge { NoMAK, MAK };
enum SlaveAcknowkedge { NoSAK, SAK, NonStandardNoSAK };

class UnioAnalyzerSettings : public AnalyzerSettings
{
public:
	UnioAnalyzerSettings();
	virtual ~UnioAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();  //Get the settings out of the interfaces, validate them, and save them to your local settings vars.
	virtual void LoadSettings( const char* settings );  //Load your settings from a string.
	virtual const char* SaveSettings();  //Save your settings to a string.

	void UpdateInterfacesFromSettings();

	Channel mScioChannel;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel > mScioChannelInterface;

};
#endif //UNIO_ANALYZER_SETTINGS
