#ifndef HDMICEC_ANALYZER_SETTINGS
#define HDMICEC_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>
#include <string>

class HdmiCecAnalyzerSettings : public AnalyzerSettings
{
public:
    HdmiCecAnalyzerSettings();
    virtual ~HdmiCecAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    void UpdateInterfacesFromSettings();
    virtual void LoadSettings( const char* settings );
    virtual const char* SaveSettings();

    Channel mCecChannel;

protected:
    std::auto_ptr< AnalyzerSettingInterfaceChannel >	mCecChannelInterface;
};

#endif //HDMICEC_ANALYZER_SETTINGS
