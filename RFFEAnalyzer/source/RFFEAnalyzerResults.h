#ifndef RFFE_ANALYZER_RESULTS
#define RFFE_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#ifdef _MSC_VER
    #define snprintf sprintf_s
#endif

#define RFFE_PARITY_ERROR_FLAG (0x1 << 0)
#define RFFE_INVALID_CMD_ERROR_FLAG (0x1 << 1)
#define RFFE_ADDR_PARITY_ERROR_FLAG (0x1 << 2)
#define RFFE_DATA_PARITY_ERROR_FLAG (0x1 << 3)
#define RFFE_NO_SSC_ERROR_FLAG (0x1 << 4)

class RFFEAnalyzer;
class RFFEAnalyzerSettings;

class RFFEAnalyzerResults : public AnalyzerResults
{
public:
    RFFEAnalyzerResults( RFFEAnalyzer* analyzer, RFFEAnalyzerSettings* settings );
    virtual ~RFFEAnalyzerResults();

    virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
    virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

    virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
    virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
    virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

public:
    enum RffeFrameType
    {
        RffeSSCField,
        RffeSAField,
        RffeTypeField,
        RffeExByteCountField,
        RffeExLongByteCountField,
        RffeShortAddressField,
        RffeAddressField,
        RffeShortDataField,
        RffeDataField,
        RffeParityField,
        RffeBusParkField,
        RffeErrorCaseField,
    };
    enum RffeTypeFieldType
    {
        RffeTypeExtWrite,
        RffeTypeReserved,
        RffeTypeExtRead,
        RffeTypeExtLongWrite,
        RffeTypeExtLongRead,
        RffeTypeNormalWrite,
        RffeTypeNormalRead,
        RffeTypeWrite0,
    };
    enum RffeAddressFieldSubType
    {
        RffeAddressNormalField,
        RffeAddressHiField,
        RffeAddressLoField,
    };

protected: //functions

protected:  //vars
    RFFEAnalyzerSettings* mSettings;
    RFFEAnalyzer* mAnalyzer;
};

#endif //RFFE_ANALYZER_RESULTS
