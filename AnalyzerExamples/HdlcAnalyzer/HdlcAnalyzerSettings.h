#ifndef HDLC_ANALYZER_SETTINGS
#define HDLC_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

/////////////////////////////////////

// NOTE: terminology:
//    * HDLC Frame == Saleae Logic Packet
//    * HDLC Field == Saleae Logic Frame
//    * HDLC transactions not supported

// Inner frames types of HDLC frame (address, control, data, fcs, etc)
enum HdlcFieldType { HDLC_FIELD_FLAG = 0, HDLC_FIELD_BASIC_ADDRESS, HDLC_FIELD_EXTENDED_ADDRESS,
                     HDLC_FIELD_BASIC_CONTROL, HDLC_FIELD_EXTENDED_CONTROL,
                     HDLC_FIELD_INFORMATION, HDLC_FIELD_FCS, HDLC_ABORT_SEQ
                   };
// Transmission mode (bit stuffing or byte stuffing)
enum HdlcTransmissionModeType { HDLC_TRANSMISSION_BIT_SYNC = 0, HDLC_TRANSMISSION_BYTE_ASYNC };
// Types of HDLC frames (Information, Supervisory and Unnumbered)
enum HdlcFrameType { HDLC_I_FRAME = 0, HDLC_S_FRAME = 1, HDLC_U_FRAME = 3 };
// Address Field type
enum HdlcAddressType { HDLC_BASIC_ADDRESS_FIELD = 0, HDLC_EXTENDED_ADDRESS_FIELD };
// Control Field Type
enum HdlcControlType { HDLC_BASIC_CONTROL_FIELD,
                       HDLC_EXTENDED_CONTROL_FIELD_MOD_128,
                       HDLC_EXTENDED_CONTROL_FIELD_MOD_32768,
                       HDLC_EXTENDED_CONTROL_FIELD_MOD_2147483648
                     };
// Frame Check Sequence algorithm
enum HdlcFcsType { HDLC_CRC8 = 0, HDLC_CRC16 = 1, HDLC_CRC32 = 2 };
// Flag Field Type (Start, End or Fill)
enum HdlcFlagType { HDLC_FLAG_START = 0, HDLC_FLAG_END = 1, HDLC_FLAG_FILL = 2 };


// Special values for Byte Asynchronous Transmission
#define HDLC_FLAG_VALUE 0x7E
#define HDLC_ESCAPE_SEQ_VALUE 0x7D
#define HDLC_FILL_VALUE 0xFF
// For Frame::mFlag
#define HDLC_ESCAPED_BYTE ( 1 << 0 )

/////////////////////////////////////

class HdlcAnalyzerSettings : public AnalyzerSettings
{
public:
	HdlcAnalyzerSettings();
	virtual ~HdlcAnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings ( const char* settings );
	virtual const char* SaveSettings();

	static U8 Bit5Inv ( U8 value );

	Channel mInputChannel;
	U32 mBitRate;

	HdlcTransmissionModeType mTransmissionMode;
	HdlcAddressType mHdlcAddr;
	HdlcControlType mHdlcControl;
	HdlcFcsType mHdlcFcs;

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel >	mInputChannelInterface;
	std::auto_ptr< AnalyzerSettingInterfaceInteger >	mBitRateInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mHdlcAddrInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mHdlcTransmissionInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mHdlcControlInterface;
	std::auto_ptr< AnalyzerSettingInterfaceNumberList >	mHdlcFcsInterface;

};

#endif //HDLC_ANALYZER_SETTINGS
