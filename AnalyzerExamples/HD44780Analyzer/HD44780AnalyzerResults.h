#ifndef HD44780_ANALYZER_RESULTS
#define HD44780_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class HD44780Analyzer;
class HD44780AnalyzerSettings;

class HD44780AnalyzerResults : public AnalyzerResults
{
public:
	HD44780AnalyzerResults( HD44780Analyzer* analyzer, HD44780AnalyzerSettings* settings );
	virtual ~HD44780AnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

  void HD44780Decode(Frame *AFrame, char *AString, DisplayBase ADisplayBase );
protected: //functions

protected:  //vars
	HD44780AnalyzerSettings* mSettings;
	HD44780Analyzer* mAnalyzer;
};

#endif //HD44780_ANALYZER_RESULTS
