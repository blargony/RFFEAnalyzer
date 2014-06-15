#ifndef DMX512_ANALYZER_RESULTS
#define DMX512_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class Dmx512Analyzer;
class Dmx512AnalyzerSettings;

class Dmx512AnalyzerResults : public AnalyzerResults
{
public:
	Dmx512AnalyzerResults( Dmx512Analyzer* analyzer, Dmx512AnalyzerSettings* settings );
	virtual ~Dmx512AnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	Dmx512AnalyzerSettings* mSettings;
	Dmx512Analyzer* mAnalyzer;
};

#endif //DMX512_ANALYZER_RESULTS
