#ifndef HDMICEC_ANALYZER_RESULTS
#define HDMICEC_ANALYZER_RESULTS

#include <AnalyzerResults.h>
#include <string>

class HdmiCecAnalyzer;
class HdmiCecAnalyzerSettings;

class HdmiCecAnalyzerResults : public AnalyzerResults
{
public:
    HdmiCecAnalyzerResults( HdmiCecAnalyzer* analyzer, HdmiCecAnalyzerSettings* settings );
    virtual ~HdmiCecAnalyzerResults();

    virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
    virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

    virtual void GenerateFrameTabularText( U64 frame_index, DisplayBase display_base );
    virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
    virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

    // Generate bubbles for different frame types.
    void GenBubbleText( U64 frame_index, DisplayBase display_base, bool tabular );
    void GenStartSeqBubble();
    void GenHeaderBubble( const Frame& frame );
    void GenOpCodeBubble( const Frame& frame );
    void GenOperandBubble( const Frame& frame );
    void GenEOMBubble( const Frame& frame );
    void GenACKBubble( const Frame& frame );

    // std::string wrapper for AddResultString
    void AddResult( const std::string& str );
    // std::string wrapper for AnalyzerHelpers::GetNumberString using mDisplayBase
    std::string GetNumberString( U64 number, int bits );

protected:  //vars
    HdmiCecAnalyzerSettings* mSettings;
    HdmiCecAnalyzer* mAnalyzer;

    DisplayBase mDisplayBase;
    bool mTabular;
};

#endif //HDMICEC_ANALYZER_RESULTS
