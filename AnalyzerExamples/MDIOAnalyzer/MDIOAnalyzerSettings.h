#ifndef MDIO_ANALYZER_SETTINGS
#define MDIO_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

// enums for the values in the different MDIO frames: START, OP, DEVTYPE
enum MdioStart { C45_START = 0, C22_START };
enum MdioOpCode { C45_ADDRESS = 0, C45_WRITE = 1, C22_WRITE = 1,C22_READ = 2, C45_READ_AND_ADDR = 2, C45_READ = 3 };
enum MdioDevType { DEV_RESERVED = 0, DEV_PMD_PMA, DEV_WIS, DEV_PCS, DEV_PHY_XS, DEV_DTE_XS, DEV_OTHER };

class MDIOAnalyzerSettings : public AnalyzerSettings
{
public:
	MDIOAnalyzerSettings();
	virtual ~MDIOAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

	Channel mMdioChannel;
	Channel mMdcChannel;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mMdioChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mMdcChannelInterface;
};

#endif //MDIO_ANALYZER_SETTINGS
