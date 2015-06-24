#ifndef RFFE_ANALYZER_RESULTS
#define RFFE_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#define RFFE_PARITY_ERROR_FLAG (0x1 << 0)
#define RFFE_INVALID_CMD_ERROR_FLAG (0x1 << 1)
#define RFFE_INCOMPLETE_PACKET_ERROR_FLAG (0x1 << 2)
#define RFFE_INVALID_MASTER_ID (0x1 << 3)



class RFFEAnalyzer;
class RFFEAnalyzerSettings;

class RFFEAnalyzerResults : public AnalyzerResults {
public:
  RFFEAnalyzerResults(RFFEAnalyzer *analyzer, RFFEAnalyzerSettings *settings);
  virtual ~RFFEAnalyzerResults();

  virtual void GenerateBubbleText(U64 frame_index, Channel &channel, DisplayBase display_base);
  virtual void GenerateExportFile(const char *file, DisplayBase display_base, U32 export_type_user_id);

  virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
  virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
  virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

public:
  enum RffeFrameType {
    RffeSSCField,
    RffeSAField,
    RffeCommandField,
    RffeExByteCountField,
    RffeExLongByteCountField,
    RffeAddressField,
    RffeAddressHiField,
    RffeAddressLoField,
    RffeShortDataField,
    RffeDataField,
    RffeMasterHandoffAckField,
    RffeISIField,
    RffeIntSlotField,
    RffeParityField,
    RffeBusParkField,
    RffeErrorCaseField,
  };

  enum RffeCommandFieldType {
    RffeTypeExtWrite,
    RffeTypeReserved,
    RffeTypeMasterRead,
    RffeTypeMasterWrite,
    RffeTypeMasterHandoff,
    RffeTypeInterrupt,
    RffeTypeExtRead,
    RffeTypeExtLongWrite,
    RffeTypeExtLongRead,
    RffeTypeNormalWrite,
    RffeTypeNormalRead,
    RffeTypeWrite0,
  };
 
protected:
  RFFEAnalyzerSettings *mSettings;
  RFFEAnalyzer *mAnalyzer;
};

// Map RffeCommandFieldType to Descriptive Strings
static const char *RffeCommandFieldStringShort[] = {
  "EW", "-", "MR", "MW", "MH", "I", "ER", "EWL", "ERL", "W", "R", "W0",
};

static const char *RffeCommandFieldStringMid[] = {
  "ExtWr", "Rsvd", "MstrRd", "MstrWr", "MstrHand", "Int", "ExtRd", "ExtWrLng", "ExtRdLng", "Wr", "Rd", "Wr0",
};

#endif // RFFE_ANALYZER_RESULTS
