#ifndef ONE_WIRE_ANALYZER_RESULTS
#define ONE_WIRE_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#define FRAMING_ERROR_FLAG ( 1 << 0 )
#define PARITY_ERROR_FLAG ( 1 << 1 )

class OneWireAnalyzer;
class OneWireAnalyzerSettings;

class OneWireAnalyzerResults : public AnalyzerResults
{
public:
	OneWireAnalyzerResults( OneWireAnalyzer* analyzer, OneWireAnalyzerSettings* settings );
	virtual ~OneWireAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	OneWireAnalyzerSettings* mSettings;
	OneWireAnalyzer* mAnalyzer;
};

#endif //ONE_WIRE_ANALYZER_RESULTS
