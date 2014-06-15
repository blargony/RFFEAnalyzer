#ifndef SIMPLEPARALLEL_ANALYZER_RESULTS
#define SIMPLEPARALLEL_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class SimpleParallelAnalyzer;
class SimpleParallelAnalyzerSettings;

class SimpleParallelAnalyzerResults : public AnalyzerResults
{
public:
	SimpleParallelAnalyzerResults( SimpleParallelAnalyzer* analyzer, SimpleParallelAnalyzerSettings* settings );
	virtual ~SimpleParallelAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions

protected:  //vars
	SimpleParallelAnalyzerSettings* mSettings;
	SimpleParallelAnalyzer* mAnalyzer;
};

#endif //SIMPLEPARALLEL_ANALYZER_RESULTS
