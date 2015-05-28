#ifndef MIDI_ANALYZER_SETTINGS
#define MIDI_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class MidiAnalyzerSettings : public AnalyzerSettings
{
public:
	MidiAnalyzerSettings();
	virtual ~MidiAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();


	Channel mInputChannel;
	U32 mBitRate;
	double mStopBits;
	double mParity;
	char mDataBits;
	bool mEndianness;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mBitRateInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mStopBitsInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mParityInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mDataBitsInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mEndiannessInterface;
};

#endif //MIDI_ANALYZER_SETTINGS
