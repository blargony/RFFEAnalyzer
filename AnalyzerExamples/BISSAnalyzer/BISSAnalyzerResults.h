#ifndef BISS_ANALYZER_RESULTS
#define BISS_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class BISSAnalyzer;
class BISSAnalyzerSettings;

class BISSAnalyzerResults : public AnalyzerResults
{
public:
	BISSAnalyzerResults( BISSAnalyzer* analyzer, BISSAnalyzerSettings* settings );
	virtual ~BISSAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );
//private: 
	//int heinz;
protected: //functions

protected:  //vars
	BISSAnalyzerSettings* mSettings;
	BISSAnalyzer* mAnalyzer;
};

#endif //BISS_ANALYZER_RESULTS
